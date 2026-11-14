"""Phase 8: Indexed SQLite historical ledger from the independent C++ parking engine."""
from datetime import datetime, timedelta
from pathlib import Path
import sqlite3
import shutil
import re
import time

class HistoryError(ValueError):
    pass

class HistoryDatabase:
    def __init__(self, filename):
        self.filename = Path(filename)
        self.filename.parent.mkdir(parents=True, exist_ok=True)
        self._signature = None
        with self.connect() as db:
            db.executescript('''
                CREATE TABLE IF NOT EXISTS sessions (
                    ticket TEXT PRIMARY KEY, plate TEXT NOT NULL, owner TEXT NOT NULL,
                    type TEXT NOT NULL, slot TEXT NOT NULL, building TEXT NOT NULL,
                    floor INTEGER NOT NULL, entry INTEGER NOT NULL CHECK(entry>0),
                    exit INTEGER, fee INTEGER NOT NULL DEFAULT 0,
                    hours INTEGER NOT NULL DEFAULT 0,
                    CHECK(exit IS NULL OR exit>=entry));
                CREATE INDEX IF NOT EXISTS ix_slot_time ON sessions(slot,entry,exit);
                CREATE INDEX IF NOT EXISTS ix_entry ON sessions(entry);
                CREATE INDEX IF NOT EXISTS ix_exit ON sessions(exit);
                CREATE INDEX IF NOT EXISTS ix_plate ON sessions(plate);
                CREATE INDEX IF NOT EXISTS ix_building ON sessions(building,floor);
                CREATE TABLE IF NOT EXISTS audit_events(
                    id INTEGER PRIMARY KEY AUTOINCREMENT, event TEXT NOT NULL,
                    ticket TEXT, plate TEXT, slot TEXT, moment INTEGER NOT NULL,
                    detail TEXT NOT NULL DEFAULT '');
            ''')

    def connect(self):
        db=sqlite3.connect(str(self.filename), timeout=8)
        db.row_factory=sqlite3.Row
        db.execute('PRAGMA busy_timeout=8000')
        return db

    @staticmethod
    def normalized(v):
        slot=str(v.get('parkingSlot') or '')
        m=re.fullmatch(r'([ABC])-F([1-4])-S\d{2}',slot)
        entry=int(v.get('entryEpoch') or 0)
        if not m or entry<=0: return None
        return (str(v['ticketId']),str(v['vehicleNumber']),str(v.get('ownerName') or ''),
                str(v.get('vehicleType') or ''),slot,m.group(1),int(m.group(2)),entry,
                int(v.get('exitEpoch') or 0) or None,int(v.get('parkingFee') or 0),int(v.get('hours') or 0))

    def sync(self, state):
        """Idempotent initial V5 migration + later updates, called under Flask's operation lock."""
        active=[s['vehicle'] for s in state.get('slots',[]) if s.get('occupied') and s.get('vehicle')]
        closed=state.get('history',[])
        signature=(state.get('served',0),len(closed),
                   closed[0].get('ticketId') if closed else None,
                   tuple(sorted((v.get('ticketId'),v.get('entryEpoch')) for v in active)))
        if self._signature==signature:return
        with self.connect() as db:
            for v in closed+active:
                row=self.normalized(v)
                if not row:continue  # Do not invent missing historic timestamps
                old=db.execute('SELECT exit FROM sessions WHERE ticket=?',(row[0],)).fetchone()
                db.execute('''INSERT INTO sessions(ticket,plate,owner,type,slot,building,floor,entry,exit,fee,hours)
                    VALUES(?,?,?,?,?,?,?,?,?,?,?) ON CONFLICT(ticket) DO UPDATE SET
                    plate=excluded.plate,owner=excluded.owner,type=excluded.type,slot=excluded.slot,
                    building=excluded.building,floor=excluded.floor,entry=excluded.entry,
                    exit=COALESCE(excluded.exit,sessions.exit),
                    fee=CASE WHEN excluded.exit IS NULL THEN sessions.fee ELSE excluded.fee END,
                    hours=CASE WHEN excluded.exit IS NULL THEN sessions.hours ELSE excluded.hours END''',row)
                if old is None:
                    db.execute('INSERT INTO audit_events(event,ticket,plate,slot,moment,detail) VALUES (?,?,?,?,?,?)',
                               ('ENTRY',row[0],row[1],row[4],row[7],'C++ allocation recorded'))
                if row[8] and (old is None or old['exit'] is None):
                    db.execute('INSERT INTO audit_events(event,ticket,plate,slot,moment,detail) VALUES (?,?,?,?,?,?)',
                               ('EXIT',row[0],row[1],row[4],row[8],f'Rs.{row[9]} / {row[10]} billed hours'))
        self._signature=signature

    def log(self, event, detail):
        if not re.fullmatch(r'[A-Z_]{3,32}',event):raise HistoryError('Invalid action')
        with self.connect() as db:
            db.execute('INSERT INTO audit_events(event,moment,detail) VALUES(?,?,?)',
                       (event,int(time.time()),str(detail)[:300]))

    def reset(self):
        # A demo reset must not destroy the only copy of historical accountability data.
        backup_dir=self.filename.parent / 'sqlite_backups'
        backup_dir.mkdir(parents=True,exist_ok=True)
        dest=backup_dir / ('before-reset-'+datetime.now().strftime('%Y%m%d-%H%M%S-%f')+'.sqlite3')
        with self.connect() as original, sqlite3.connect(str(dest)) as target:
            original.backup(target)
        with self.connect() as db:
            db.execute('DELETE FROM sessions')
            db.execute('DELETE FROM audit_events')
            db.execute('INSERT INTO audit_events(event,moment,detail) VALUES(?,?,?)',
                       ('RESET',int(time.time()),'Admin reset demo data; C++ creates a backup'))
        self._signature=None

    @staticmethod
    def window(period,date):
        if period not in ('week','month') or not re.fullmatch(r'\d{4}-\d{2}-\d{2}',date or ''):
            raise HistoryError('Select week/month and use YYYY-MM-DD.')
        try: ref=datetime.strptime(date,'%Y-%m-%d')
        except ValueError as ex:raise HistoryError('Invalid calendar date.') from ex
        if period=='week':
            start=ref-timedelta(days=ref.weekday())
            end=start+timedelta(days=7)
        else:
            start=ref.replace(day=1)
            end=start.replace(year=start.year+1,month=1) if start.month==12 else start.replace(month=start.month+1)
        return int(start.timestamp()),int(end.timestamp()),start,end

    @staticmethod
    def as_json(row):
        v=dict(row)
        return {'ticketId':v['ticket'],'vehicleNumber':v['plate'],'ownerName':v['owner'],
            'vehicleType':v['type'],'parkingSlot':v['slot'],'building':v['building'],
            'floor':v['floor'],'entryEpoch':v['entry'],'exitEpoch':v['exit'] or 0,
            'parkingFee':v['fee'],'hours':v['hours']}

    def report(self,period,date,building='',floor=0,slot='',plate='',limit=250,offset=0):
        start,end,startdt,enddt=self.window(period,date)
        if building not in ('','A','B','C') or floor not in range(5):
            raise HistoryError('Invalid building/floor.')
        if slot and not re.fullmatch(r'[ABC]-F[1-4]-S\d{2}',slot):raise HistoryError('Invalid slot.')
        if not re.fullmatch(r'[A-Z0-9 -]{0,20}',plate):raise HistoryError('Invalid plate.')
        if not 1<=limit<=500 or not 0<=offset<=1000000:raise HistoryError('Invalid page.')
        # Apply the SAME administrator-selected filters to tables and all summaries.
        # Previous versions filtered rows but accidentally displayed site-wide totals.
        extra=[];extra_values=[]
        for col,val in (('building',building),('floor',floor),('slot',slot),('plate',plate)):
            if val:
                extra.append(f'{col}=?')
                extra_values.append(val)
        predicate=' AND '.join(extra)
        def condition(*parts):
            return ' AND '.join((*parts,*extra))
        overlap=condition('entry<?','(exit IS NULL OR exit>?)')
        overlap_params=(end,start,*extra_values)
        with self.connect() as db:
            count=db.execute('SELECT COUNT(*) FROM sessions WHERE '+overlap,overlap_params).fetchone()[0]
            records=db.execute('SELECT * FROM sessions WHERE '+overlap+' ORDER BY entry DESC,ticket DESC LIMIT ? OFFSET ?',
                               (*overlap_params,limit,offset)).fetchall()
            allrows=db.execute('SELECT slot,entry,exit FROM sessions WHERE '+overlap,overlap_params).fetchall()
            arrivals=db.execute('SELECT COUNT(*) FROM sessions WHERE '+condition('entry>=?','entry<?'),
                                (start,end,*extra_values)).fetchone()[0]
            paid=db.execute('SELECT COUNT(*),COALESCE(SUM(fee),0) FROM sessions WHERE '+condition('exit>=?','exit<?'),
                            (start,end,*extra_values)).fetchone()
            bybuilding=db.execute('SELECT building,COUNT(*) AS count,COALESCE(SUM(fee),0) AS revenue FROM sessions WHERE '
                                  +condition('exit>=?','exit<?')+' GROUP BY building ORDER BY building',
                                  (start,end,*extra_values)).fetchall()
        byslot={}; active=0;now=int(time.time())
        for r in allrows:
            if r['exit'] is None and r['entry']<=now:
                active+=1
            a=max(start,r['entry']);b=min(end,now,r['exit'] if r['exit'] is not None else now)
            if b>a:byslot[r['slot']]=byslot.get(r['slot'],0)+b-a
        return {'period':period,'startEpoch':start,'endEpoch':end,
                'label':f'{startdt:%Y-%m-%d} to {enddt:%Y-%m-%d} (end exclusive)',
                'totalMatching':count,'limit':limit,'offset':offset,
                'summary':{'sessions':count,'arrivals':arrivals,'departures':paid[0],
                           'revenue':paid[1],'activeNow':active,'occupiedMinutes':sum(byslot.values())//60,
                           'buildings':[{'building':r['building'],'departures':r['count'],'revenue':r['revenue']} for r in bybuilding],
                           'topSlots':[{'slot':s,'occupiedMinutes':seconds//60}
                                       for s,seconds in sorted(byslot.items(),key=lambda kv:(-kv[1],kv[0]))[:8]]},
                'rows':[self.as_json(r) for r in records]}

    def slot_at(self,slot,at):
        if not re.fullmatch(r'[ABC]-F[1-4]-S\d{2}',slot or ''):
            raise HistoryError('Enter a slot like A-F1-S02.')
        if not re.fullmatch(r'\d{4}-\d{2}-\d{2}[ T]\d{2}:\d{2}',at or ''):
            raise HistoryError('Enter YYYY-MM-DD HH:MM.')
        try:when=int(datetime.strptime(at.replace('T',' '),'%Y-%m-%d %H:%M').timestamp())
        except ValueError as ex:raise HistoryError('Invalid date/time.') from ex
        if when>time.time():raise HistoryError('Enter a past date/time.')
        with self.connect() as db:
            row=db.execute('SELECT * FROM sessions WHERE slot=? AND entry<=? AND (exit IS NULL OR exit>?) ORDER BY entry DESC LIMIT 1',
                           (slot,when,when)).fetchone()
        return {'slot':slot,'atEpoch':when,'occupied':row is not None,'vehicle':self.as_json(row) if row else None}

    def audit(self,limit=100):
        if not 1<=limit<=500:raise HistoryError('Invalid limit.')
        with self.connect() as db:
            events=db.execute('SELECT * FROM audit_events ORDER BY id DESC LIMIT ?',(limit,)).fetchall()
        return [dict(e) for e in events]
