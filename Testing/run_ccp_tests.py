"""Automated integration tests for independent SmartPark C++ bridge.
Run: python Testing/run_ccp_tests.py --bridge PATH_TO_CPP_BRIDGE
Uses temporary state files; does not alter a real parking database.
"""
import argparse
import json
from pathlib import Path
import subprocess
import tempfile
import unittest

PARSER = argparse.ArgumentParser(add_help=True)
PARSER.add_argument('--bridge', required=True)
ARGS, REMAINDER = PARSER.parse_known_args()
ENGINE = str(Path(ARGS.bridge).resolve())


class SmartParkTest(unittest.TestCase):
    def setUp(self):
        self.folder = tempfile.TemporaryDirectory()
        self.addCleanup(self.folder.cleanup)
        self.path = str(Path(self.folder.name)/'parking.txt')

    def call(self, name, *args, okay=True):
        process = subprocess.run([ENGINE, self.path, name, *map(str, args)],
                                 text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=12)
        try:
            result = json.loads(process.stdout)
        except ValueError:
            self.fail(f'Invalid JSON {name}: {process.returncode} {process.stdout} {process.stderr}')
        if okay:
            self.assertEqual(process.returncode,0,(name,result))
        else:
            self.assertNotEqual(process.returncode,0,(name,result))
        return result

    def convoy(self, prefix='CAR', size=4, building='A', floor=1, vip=False):
        parts=[]
        for i in range(size):
            parts.extend([f'{prefix}-{i+1}', 'Car', f'Family {i+1}'])
        return self.call('convoy',size,building,floor,int(vip),*parts)

    def test_find_my_car_raw_avl_and_stack(self):
        self.call('park','ABC-123','Car','Driver',0,'A',1)
        found=self.call('find-my-car','abc-123')
        self.assertTrue(found['found'])
        self.assertTrue(found['reachable'])
        self.assertEqual(found['path'].split(' -> ')[0],'PED-GATE')
        self.assertEqual(found['path'].split(' -> ')[-1],found['slot'])
        self.assertEqual(len(found['steps'].splitlines()),found['edges'])
        self.assertGreaterEqual(found['comparisons'],1)

    def test_find_car_missing_and_blocked(self):
        self.assertFalse(self.call('find-my-car','MISSING-1')['found'])
        self.call('park','VEH-123','Car','Driver',0,'A',1)
        for building in 'ABC': self.call('road-block','ENTRY',f'G-{building}',1)
        result=self.call('find-my-car','VEH-123')
        self.assertTrue(result['found']); self.assertFalse(result['reachable'])

    def test_vip_hold_rejects_duplicate_and_excludes_allocation(self):
        hold=self.call('vip-reserve','VIP-101','Car','Owner','A',1)
        snapshot=self.call('state')
        held=[s for s in snapshot['slots'] if s['id']==hold['slot']][0]
        self.assertTrue(held['held']);self.assertFalse(held['occupied'])
        self.call('vip-reserve','VIP-101','Car','Owner','A',1,okay=False)
        self.call('park','VIP-101','Car','Owner',1,'A',1,okay=False)
        parked=self.call('park','OTHER-101','Car','Driver',0,'A',1)
        self.assertNotEqual(parked['vehicle']['parkingSlot'],hold['slot'])

    def test_vip_road_rollback_reroute_and_confirm(self):
        old=self.call('vip-reserve','VIP-201','Car','Owner','A',1)
        self.call('road-block','ENTRY','G-A',1)
        new=self.call('ccp-state')
        self.assertIn('Rollback',new['lastRecovery'])
        self.assertEqual(len(new['holds']),1)
        self.assertIn('G-B',new['holds'][0]['route'])
        self.call('vip-confirm','VIP-201')
        self.assertTrue(self.call('find-my-car','VIP-201')['found'])
        self.assertEqual(self.call('ccp-state')['holds'],[])
        self.assertTrue(self.call('health')['treesValid'])

    def test_vip_no_alternate_moves_into_priority_queue(self):
        self.call('vip-reserve','VIP-202','Car','Owner','A',1)
        self.call('road-block','G-A','G-B',1)
        self.call('road-block','ENTRY','G-A',1)
        state=self.call('ccp-state')
        self.assertEqual(state['holds'],[])
        self.assertIn('Priority Queue',state['lastRecovery'])
        self.assertIn('VIP-202',[v['vehicleNumber'] for v in self.call('state')['waiting']])

    def test_four_car_contiguous(self):
        result=self.convoy('CNV')
        self.assertFalse(result['queued'])
        self.assertFalse(result['split'])
        self.assertEqual(len(result['slots']),4)
        self.assertEqual([int(slot[-2:]) for slot in result['slots']],[2,3,4,5])
        self.assertEqual(self.call('state')['occupied'],4)
        self.assertTrue(self.call('health')['treesValid'])

    def test_four_car_smart_split_2plus2(self):
        self.call('slot-rule','A-F1-S04',2,'Lane maintenance')
        result=self.convoy('SPLIT')
        self.assertTrue(result['split'])
        self.assertEqual([int(slot[-2:]) for slot in result['slots']],[2,3,5,6])
        self.assertEqual(self.call('state')['occupied'],4)

    def test_atomic_group_queue_and_reassignment(self):
        for index in range(2,7): self.call('slot-rule',f'A-F1-S{index:02d}',2,'maintenance')
        result=self.convoy('WAIT')
        self.assertTrue(result['queued'])
        self.assertEqual(self.call('state')['occupied'],0)
        self.assertEqual(self.call('ccp-state')['groups'][0]['count'],4)
        for index in range(2,7): self.call('slot-rule',f'A-F1-S{index:02d}',0,'')
        self.assertEqual(self.call('ccp-state')['groups'],[])
        self.assertEqual(self.call('state')['occupied'],4)

    def test_group_duplicate_is_rejected_without_partial_parking(self):
        self.call('convoy',3,'',0,0,'DUP-1','Car','A','DUP-1','Car','B','DUP-3','Car','C',okay=False)
        self.assertEqual(self.call('state')['occupied'],0)

    def test_evacuate_routes_and_freeze_normal_arrivals(self):
        self.call('park','A-1','Car','Alpha',0,'A',1)
        state=self.call('evacuate','A')
        self.assertTrue(state['active'])
        self.assertEqual(state['evacuationRoutes'][0]['exitGate'],'EXIT-B')
        self.call('park','NORMAL-22','Car','Blocked',0,'B',1,okay=False)
        self.call('convoy',3,'B',1,0,'C1','Car','One','C2','Car','Two','C3','Car','Three',okay=False)
        self.call('park','EMER-22','Emergency','Ambulance',1,'B',1)
        self.assertTrue(self.call('ccp-state')['active'])
        self.call('evac-stop')
        self.call('park','NORMAL-22','Car','Allowed',0,'B',1)

    def test_evacuate_no_safe_route_requires_assistance(self):
        self.call('park','A-2','Car','Driver',0,'A',1)
        self.call('road-block','G-A','G-B',1)
        self.call('road-block','ENTRY','G-A',1)
        result=self.call('evacuate','A')
        self.assertFalse(result['evacuationRoutes'][0]['reachable'])
        self.assertIn('NO SAFE ROUTE',result['evacuationRoutes'][0]['path'])

    def test_evacuate_blocks_confirmation_in_affected_building(self):
        self.call('vip-reserve','VIP-E1','Car','Driver','A',1)
        self.call('evacuate','A')
        self.call('vip-confirm','VIP-E1',okay=False)
        self.assertEqual(self.call('state')['occupied'],0)

    def test_evacuate_state_survives_restart(self):
        self.call('evacuate','B')
        self.assertEqual(self.call('ccp-state')['building'],'B')
        self.call('park','NORM-B','Car','Blocked',0,'C',1,okay=False)
        self.call('evac-stop')
        self.assertFalse(self.call('ccp-state')['active'])

    def test_queue_persist_after_restart(self):
        for index in range(2,7):self.call('slot-rule',f'C-F1-S{index:02d}',2,'closed')
        result=self.convoy('PERSIST',4,'C',1)
        self.assertTrue(result['queued'])
        self.assertEqual(self.call('ccp-state')['groups'][0]['id'],result['groupId'])
        content=Path(self.path).read_text()
        self.assertIn('SMARTPARK_CPP_V6',content)
        self.assertIn('GROUPS 1',content)

    def test_vip_hold_after_restart(self):
        self.call('vip-reserve','PERSIST-H','SUV','Owner','C',2)
        state=self.call('ccp-state')
        self.assertEqual(len(state['holds']),1)
        self.call('vip-confirm','PERSIST-H')
        self.assertTrue(self.call('health')['treesValid'])

    def test_legacy_v5_migration(self):
        self.call('park','LEGACY-1','Car','Driver',0,'A',1)
        content=Path(self.path).read_text()
        self.assertIn('CCP 0',content)
        import re
        legacy=re.sub(r'CCP 0[^\n]*\nHOLDS 0\nGROUPS 0\n','',content).replace('SMARTPARK_CPP_V6','SMARTPARK_CPP_V5')
        Path(self.path).write_text(legacy)
        self.assertEqual(self.call('state')['occupied'],1)
        self.assertTrue(self.call('find-my-car','LEGACY-1')['found'])

    def test_existing_billing_and_exit_still_works(self):
        entered=self.call('park','OLD-11','Car','Driver',0,'A',1)
        self.assertFalse(entered['queued'])
        exited=self.call('exit','OLD-11')
        self.assertIn('bill',exited)
        self.assertEqual(self.call('state')['occupied'],0)
        self.assertFalse(self.call('find-my-car','OLD-11')['found'])
        self.assertTrue(self.call('health')['treesValid'])

    def test_reserve_capacity_and_hold_prevent_duplicate(self):
        first=self.call('vip-reserve','HOLD-1','Car','Owner','A',1)
        second=self.call('vip-reserve','HOLD-2','Car','Owner','A',1)
        self.assertNotEqual(first['slot'],second['slot'])


if __name__=='__main__':
    unittest.main(argv=['run_ccp_tests.py',*REMAINDER],verbosity=2)
