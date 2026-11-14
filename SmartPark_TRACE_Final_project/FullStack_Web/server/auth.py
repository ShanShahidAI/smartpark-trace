"""Phase 1 auth for SmartPark TRACE website.
First-run account setup. Password hashes use Werkzeug scrypt and cookies use Flask sessions.
"""
import hmac
import os
import secrets
from functools import wraps
from pathlib import Path
from threading import Lock

from flask import abort, jsonify, redirect, render_template, request, session, url_for
from werkzeug.security import check_password_hash, generate_password_hash
import json

_FAILED = {}
_FAILED_LOCK = Lock()


def install_auth(app, data_dir):
    data_dir = Path(data_dir)
    data_dir.mkdir(parents=True, exist_ok=True)
    credential_file = data_dir / 'admin_web.json'
    key_file = data_dir / '.web_session_secret'
    if not key_file.exists():
        # Local-only session signing secret, independent of standalone C++.
        with key_file.open('x', encoding='utf-8') as file:
            file.write(secrets.token_hex(32))
        try: os.chmod(key_file, 0o600)
        except OSError: pass
    app.config['SECRET_KEY'] = key_file.read_text(encoding='utf-8').strip()
    app.config.update(SESSION_COOKIE_HTTPONLY=True, SESSION_COOKIE_SAMESITE='Lax')

    def configured():
        return credential_file.is_file()

    def load_admin():
        try: return json.loads(credential_file.read_text(encoding='utf-8'))
        except (OSError, ValueError): return None

    def new_token():
        token = secrets.token_urlsafe(32)
        session['csrf_token'] = token
        return token

    @app.before_request
    def require_admin():
        path = request.path
        if path.startswith('/static/') or path in ('/setup', '/login'):
            return None
        if not configured():
            if path.startswith('/api/'):
                return jsonify(error='Admin setup is required.'), 401
            return redirect('/setup')
        if session.get('admin') != (load_admin() or {}).get('username'):
            if path.startswith('/api/'):
                return jsonify(error='Admin login required.'), 401
            return redirect('/login')
        if request.method in ('POST', 'PUT', 'PATCH', 'DELETE'):
            supplied = request.headers.get('X-CSRF-Token') if path.startswith('/api/') else request.form.get('csrf_token')
            if not hmac.compare_digest(supplied or '', session.get('csrf_token', '')):
                return (jsonify(error='Invalid session token.'), 403) if path.startswith('/api/') else ('Invalid session token.', 403)

    @app.route('/setup', methods=['GET', 'POST'])
    def setup():
        if configured():
            return redirect('/login')
        error = ''
        if request.method == 'POST':
            name = request.form.get('username', '').strip()
            password = request.form.get('password', '')
            confirmation = request.form.get('confirm', '')
            if not (3 <= len(name) <= 32 and name.replace('_', '').isalnum()):
                error = 'Use 3-32 letters, numbers, or underscores for username.'
            elif len(password) < 8 or password != confirmation:
                error = 'Password must have 8+ characters and match confirmation.'
            else:
                data = {'username': name, 'password_hash': generate_password_hash(password, method='scrypt')}
                try:
                    # Atomic first creation: cannot overwrite an existing admin file.
                    with credential_file.open('x', encoding='utf-8') as file:
                        json.dump(data, file)
                    try: os.chmod(credential_file, 0o600)
                    except OSError: pass
                    return redirect('/login?created=1')
                except FileExistsError:
                    return redirect('/login')
        return render_template('auth.html', mode='setup', error=error)

    @app.route('/login', methods=['GET', 'POST'])
    def login():
        if not configured(): return redirect('/setup')
        if session.get('admin') == (load_admin() or {}).get('username'):
            return redirect('/')
        error = ''
        if request.method == 'POST':
            username = request.form.get('username', '').strip()
            password = request.form.get('password', '')
            client_ip = request.remote_addr or 'local'
            with _FAILED_LOCK:
                count = _FAILED.get(client_ip, 0)
            if count >= 5:
                error = 'Too many unsuccessful attempts. Restart local server to retry.'
            else:
                admin = load_admin()
                if admin and hmac.compare_digest(username, admin['username']) and check_password_hash(admin['password_hash'], password):
                    with _FAILED_LOCK: _FAILED.pop(client_ip, None)
                    session.clear()
                    session['admin'] = username
                    new_token()
                    return redirect('/')
                with _FAILED_LOCK: _FAILED[client_ip] = count + 1
                error = 'Incorrect username or password.'
        return render_template('auth.html', mode='login', error=error)

    @app.post('/logout')
    def logout():
        session.clear()
        return redirect('/login')

    @app.get('/api/auth/session')
    def auth_session():
        return jsonify(username=session['admin'], csrfToken=session['csrf_token'])
