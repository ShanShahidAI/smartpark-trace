/* SmartPark TRACE — navigational presentation only; parking logic remains in C++ */
(() => {
  'use strict';
  const body = document.body;
  const collapse = document.getElementById('sidebarCollapse');
  const side = document.getElementById('Sidebar');
  const desktopQuery = window.matchMedia('(min-width: 951px)');
  const KEY = 'smartpark-trace-sidebar-collapsed';
  const apply = collapsed => {
    body.classList.toggle('sidebar-collapsed', !!collapsed);
    if (collapse) {
      collapse.setAttribute('aria-expanded', String(!collapsed));
      collapse.setAttribute('aria-label', collapsed ? 'Expand sidebar' : 'Collapse sidebar');
      collapse.title = collapsed ? 'Expand navigation' : 'Collapse navigation';
    }
  };
  try { if (localStorage.getItem(KEY) === 'true' && desktopQuery.matches) apply(true); } catch (_) {}
  collapse?.addEventListener('click', () => {
    if (!desktopQuery.matches) return;
    const collapsed = !body.classList.contains('sidebar-collapsed');
    apply(collapsed);
    try { localStorage.setItem(KEY,String(collapsed)); } catch (_) {}
  });
  if (side) {
    side.querySelectorAll('nav .nav').forEach(btn => {
      btn.title = btn.innerText.trim();
      btn.addEventListener('click', () => {
        if (desktopQuery.matches) side.querySelector('nav')?.scrollTo({top:Math.max(0,btn.offsetTop-160),behavior:'smooth'});
      });
    });
  }
  document.getElementById('phase9Backdrop')?.addEventListener('click', () => body.classList.remove('phase9-menu-open'));
  const menuButton = document.getElementById('phase9MenuBtn');
  menuButton?.addEventListener('click', () => { if (body.classList.contains('phase9-menu-open')) side?.querySelector('.nav.active')?.focus({preventScroll:true}); });
  desktopQuery.addEventListener?.('change', e => {
    body.classList.remove('phase9-menu-open');
    if (e.matches) {
      try { apply(localStorage.getItem(KEY)==='true'); }catch(_){apply(false);}
    } else { apply(false); }
  });
})();
