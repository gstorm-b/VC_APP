
// DockArea.jsx - Tab bar, bottom dock, floating windows
const { useState, useRef, useEffect, useCallback } = React;

const DOCK_TASK_COLORS = { LocalizationTask:'#2b8ce8', PickPlaceTask:'#22d17a', InspectTask:'#f5a623' };
const DOCK_TASK_CHIPS  = { LocalizationTask:'LOC', PickPlaceTask:'PICK', InspectTask:'INSP' };

/* ─────────────────────────────────────────
   Task Tab Bar
───────────────────────────────────────── */
function TaskTabBar({ openTasks, activeTaskId, onSelect, onClose, onAddTab }) {
  return (
    <div style={{
      height:36, background:'#1e1e1e', borderBottom:'1px solid #2d2d2d',
      display:'flex', alignItems:'stretch', overflow:'hidden', flexShrink:0,
    }}>
      <div style={{ display:'flex', flex:1, overflowX:'auto', overflowY:'hidden' }}>
        {openTasks.map(task => {
          const active = task.id === activeTaskId;
          const color  = DOCK_TASK_COLORS[task.type] || '#9a9a9a';
          const chip   = DOCK_TASK_CHIPS[task.type]  || '?';
          return (
            <div
              key={task.id}
              onClick={() => onSelect(task.id)}
              style={{
                display:'flex', alignItems:'center', gap:7,
                padding:'0 14px 0 12px', cursor:'pointer', flexShrink:0,
                borderRight:'1px solid #2d2d2d',
                background: active ? '#252526' : 'transparent',
                borderTop: active ? `2px solid ${color}` : '2px solid transparent',
                borderBottom: active ? '1px solid #252526' : '1px solid transparent',
                position:'relative', transition:'background 0.12s',
                userSelect:'none', minWidth:140, maxWidth:220,
              }}
              onMouseEnter={e=>{ if(!active) e.currentTarget.style.background='#2a2d2e'; }}
              onMouseLeave={e=>{ if(!active) e.currentTarget.style.background='transparent'; }}
            >
              {/* task type chip */}
              <span style={{
                fontSize:8, fontWeight:800, color, background:`${color}18`,
                border:`1px solid ${color}44`, borderRadius:3, padding:'1px 5px',
                letterSpacing:'0.06em', flexShrink:0,
              }}>{chip}</span>
              {/* task name */}
              <span style={{
                fontSize:11, fontFamily:'JetBrains Mono, monospace', fontWeight:active?700:400,
                color:active?'#cccccc':'#9a9a9a',
                overflow:'hidden', textOverflow:'ellipsis', whiteSpace:'nowrap', flex:1,
              }}>{task.name}</span>
              {/* device count */}
              <span style={{ fontSize:9, color:'#454545', fontFamily:'JetBrains Mono, monospace', flexShrink:0 }}>
                {task.devices.length}d
              </span>
              {/* close button */}
              <button
                onClick={e=>{ e.stopPropagation(); onClose(task.id); }}
                style={{ background:'transparent', border:'none', cursor:'pointer', color:'transparent', padding:'1px', borderRadius:3, display:'flex', flexShrink:0 }}
                onMouseEnter={e=>{ e.currentTarget.style.color='#e84040'; e.currentTarget.style.background='#2a1a1a'; }}
                onMouseLeave={e=>{ e.currentTarget.style.color='transparent'; e.currentTarget.style.background='transparent'; }}
              ><IcoX size={11}/></button>
            </div>
          );
        })}

        {/* Add tab button */}
        <button onClick={onAddTab} style={{
          background:'transparent', border:'none', cursor:'pointer', padding:'0 12px',
          color:'#454545', display:'flex', alignItems:'center', flexShrink:0,
          borderRight:'1px solid #2d2d2d',
        }}
          onMouseEnter={e=>{ e.currentTarget.style.color='#2b8ce8'; e.currentTarget.style.background='#2a2d2e'; }}
          onMouseLeave={e=>{ e.currentTarget.style.color='#454545'; e.currentTarget.style.background='transparent'; }}
          title="Open task in new tab"
        ><IcoPlus size={14}/></button>
      </div>

      {/* Right: dock action buttons */}
      <div style={{ display:'flex', alignItems:'center', gap:2, padding:'0 8px', borderLeft:'1px solid #2d2d2d', flexShrink:0 }}>
        {[
          { Icon: IcoCamera, title:'Float Camera Feed', id:'camera-feed' },
          { Icon: IcoPattern, title:'Float Pattern Library', id:'pattern-lib' },
        ].map(({Icon,title,id})=>(
          <button key={id} title={title} style={{ background:'transparent', border:'none', cursor:'pointer', color:'#454545', padding:'4px 6px', borderRadius:4, display:'flex', alignItems:'center', gap:5 }}
            onMouseEnter={e=>{ e.currentTarget.style.color='#9a9a9a'; e.currentTarget.style.background='#2d2d2d'; }}
            onMouseLeave={e=>{ e.currentTarget.style.color='#454545'; e.currentTarget.style.background='transparent'; }}
          ><Icon size={13}/></button>
        ))}
      </div>
    </div>
  );
}

/* ─────────────────────────────────────────
   System Log (bottom panel)
───────────────────────────────────────── */
const BOTTOM_LOGS = [
  { time:'09:41:22', level:'OK',   msg:'Task Task_Loc_01 initialized' },
  { time:'09:41:23', level:'INFO', msg:'Camera Basler acA2440 connected — 192.168.1.10' },
  { time:'09:41:23', level:'INFO', msg:'PLC Mitsubishi connected — 192.168.1.20:5007' },
  { time:'09:41:25', level:'OK',   msg:'READY signal → PLC M100=ON' },
  { time:'09:42:01', level:'INFO', msg:'Trigger received M0=ON' },
  { time:'09:42:01', level:'OK',   msg:'Match OK — score:0.967 pos:(823,512) angle:12.4°' },
  { time:'09:42:02', level:'INFO', msg:'Output → D100:8230 D101:5120 D102:124 D103:967' },
  { time:'09:43:18', level:'WARN', msg:'Low match score:0.521 pattern #3 — retry' },
  { time:'09:43:19', level:'OK',   msg:'Retry #1 — score:0.882 accepted' },
  { time:'09:44:05', level:'INFO', msg:'Task Task_Pick_01 device Robot_Fanuc_01 disconnected' },
  { time:'09:44:10', level:'ERROR',msg:'PLC_Mitsu_02 timeout after 1000ms — retry 1/2' },
  { time:'09:44:11', level:'WARN', msg:'PLC_Mitsu_02 reconnected after 1120ms' },
];
const LOG_C = { OK:'#22d17a', INFO:'#9cdcfe', WARN:'#f5a623', ERROR:'#e84040' };

function SystemLogPanel() {
  const endRef = useRef(null);
  return (
    <div style={{ height:'100%', display:'flex', flexDirection:'column', overflow:'hidden' }}>
      <div style={{ display:'flex', alignItems:'center', padding:'4px 12px', gap:8, borderBottom:'1px solid #2d2d2d', flexShrink:0 }}>
        <span style={{ fontSize:9, color:'#454545', fontWeight:700, textTransform:'uppercase', letterSpacing:'0.1em', flex:1 }}>System Log</span>
        <button style={{ background:'transparent', border:'none', cursor:'pointer', color:'#454545', padding:'2px 6px', borderRadius:3, fontSize:9, fontWeight:700, display:'flex', alignItems:'center', gap:4 }}>
          <IcoRefresh size={10}/> Clear
        </button>
        {['ALL','OK','WARN','ERROR'].map(f=>(
          <button key={f} style={{ background:'transparent', border:'1px solid #2d2d2d', borderRadius:3, padding:'1px 7px', fontSize:9, color:'#5a5a5a', cursor:'pointer', fontWeight:700 }}
            onMouseEnter={e=>e.currentTarget.style.borderColor='#2b8ce8'} onMouseLeave={e=>e.currentTarget.style.borderColor='#2d2d2d'}
          >{f}</button>
        ))}
      </div>
      <div style={{ flex:1, overflowY:'auto', padding:'4px 0' }}>
        {BOTTOM_LOGS.map((l,i)=>(
          <div key={i} style={{ display:'flex', gap:10, padding:'3px 12px', fontFamily:'JetBrains Mono, monospace', fontSize:11 }}>
            <span style={{ color:'#454545', flexShrink:0, width:56 }}>{l.time}</span>
            <span style={{ color:LOG_C[l.level]||'#9a9a9a', flexShrink:0, width:34, fontWeight:700 }}>{l.level}</span>
            <span style={{ color:'#9a9a9a' }}>{l.msg}</span>
          </div>
        ))}
        <div ref={endRef}/>
      </div>
    </div>
  );
}

/* ─────────────────────────────────────────
   PLC Register Monitor (bottom panel)
───────────────────────────────────────── */
const PLC_WATCH = [
  { addr:'M0',   name:'Trigger',     val:0, type:'bit' },
  { addr:'M100', name:'Ready',       val:1, type:'bit' },
  { addr:'M101', name:'Complete',    val:1, type:'bit' },
  { addr:'M102', name:'Match OK',    val:1, type:'bit' },
  { addr:'M103', name:'Match NG',    val:0, type:'bit' },
  { addr:'D100', name:'Pos X',       val:8230, type:'word', unit:'×0.1mm' },
  { addr:'D101', name:'Pos Y',       val:5120, type:'word', unit:'×0.1mm' },
  { addr:'D102', name:'Angle',       val:124,  type:'word', unit:'×0.1°' },
  { addr:'D103', name:'Score',       val:967,  type:'word', unit:'×0.001' },
  { addr:'D104', name:'Pattern ID',  val:1,    type:'word' },
  { addr:'D105', name:'Cycle (ms)',  val:124,  type:'word', unit:'ms' },
];

function PLCMonitorPanel() {
  const [regs, setRegs] = useState(PLC_WATCH);
  const [connected, setConnected] = useState(true);

  // Simulate live updates
  useEffect(()=>{
    if(!connected) return;
    const t = setInterval(()=>{
      setRegs(prev=>prev.map(r=>{
        if(r.type==='word' && (r.addr==='D100'||r.addr==='D101')) return {...r, val: r.val + Math.floor((Math.random()-0.5)*5)};
        if(r.addr==='D105') return {...r, val: 115+Math.floor(Math.random()*30)};
        return r;
      }));
    }, 1800);
    return ()=>clearInterval(t);
  }, [connected]);

  return (
    <div style={{ height:'100%', display:'flex', flexDirection:'column', overflow:'hidden' }}>
      <div style={{ display:'flex', alignItems:'center', padding:'4px 12px', gap:8, borderBottom:'1px solid #2d2d2d', flexShrink:0 }}>
        <span style={{ fontSize:9, color:'#454545', fontWeight:700, textTransform:'uppercase', letterSpacing:'0.1em', flex:1 }}>PLC Register Monitor</span>
        <div style={{ display:'flex', alignItems:'center', gap:5 }}>
          <div style={{ width:6, height:6, borderRadius:'50%', background:connected?'#22d17a':'#e84040', boxShadow:connected?'0 0 5px #22d17a':'' }}/>
          <span style={{ fontSize:9, color:connected?'#22d17a':'#e84040', fontWeight:700 }}>{connected?'LIVE':'OFFLINE'}</span>
        </div>
        <button onClick={()=>setConnected(c=>!c)} style={{ background:connected?'#2a1a1a':'#3c3c3c', border:`1px solid ${connected?'#e8404044':'#454545'}`, borderRadius:3, padding:'2px 8px', fontSize:9, color:connected?'#e84040':'#9a9a9a', cursor:'pointer', fontWeight:700 }}>
          {connected?'Stop':'Connect'}
        </button>
        <button style={{ background:'transparent', border:'none', cursor:'pointer', color:'#454545', padding:'2px', display:'flex' }}><IcoPlus size={12}/></button>
      </div>
      <div style={{ flex:1, overflowX:'auto', overflowY:'auto' }}>
        <table style={{ width:'100%', borderCollapse:'collapse', fontFamily:'JetBrains Mono, monospace', fontSize:11 }}>
          <thead style={{ position:'sticky', top:0, background:'#1e1e1e', zIndex:1 }}>
            <tr>
              {['Address','Name','Value','Type','Unit'].map(h=>(
                <th key={h} style={{ padding:'4px 10px', textAlign:'left', fontSize:9, color:'#454545', fontWeight:700, textTransform:'uppercase', letterSpacing:'0.08em', borderBottom:'1px solid #2d2d2d', whiteSpace:'nowrap' }}>{h}</th>
              ))}
            </tr>
          </thead>
          <tbody>
            {regs.map((r,i)=>(
              <tr key={r.addr} style={{ borderBottom:'1px solid #2a2d2e', background:i%2===0?'transparent':'#252526' }}>
                <td style={{ padding:'4px 10px', color:'#9cdcfe', fontWeight:600 }}>{r.addr}</td>
                <td style={{ padding:'4px 10px', color:'#7a7a7a' }}>{r.name}</td>
                <td style={{ padding:'4px 10px' }}>
                  {r.type==='bit' ? (
                    <span style={{ display:'inline-block', padding:'1px 8px', borderRadius:3, fontSize:9, fontWeight:800, letterSpacing:'0.06em', background:r.val?'#22d17a22':'#3c3c3c', color:r.val?'#22d17a':'#5a5a5a', border:`1px solid ${r.val?'#22d17a44':'#3c3c3c'}` }}>
                      {r.val?'ON':'OFF'}
                    </span>
                  ) : (
                    <span style={{ color:'#cccccc', fontWeight:600 }}>{r.val}</span>
                  )}
                </td>
                <td style={{ padding:'4px 10px', color:'#454545', fontSize:9, textTransform:'uppercase' }}>{r.type}</td>
                <td style={{ padding:'4px 10px', color:'#454545', fontSize:9 }}>{r.unit||'—'}</td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>
    </div>
  );
}

/* ─────────────────────────────────────────
   Bottom Dock
───────────────────────────────────────── */
const BOTTOM_PANELS = [
  { id:'syslog',     label:'System Log',       Icon:IcoLog     },
  { id:'plcmonitor', label:'PLC Monitor',      Icon:IcoPLC     },
];

function BottomDock({ height, onResize, onClose }) {
  const [activePanel, setActivePanel] = useState('syslog');
  const dragRef = useRef(null);

  const handleDragStart = useCallback(e => {
    dragRef.current = { startY: e.clientY, startH: height };
    const onMove = e => {
      const delta = dragRef.current.startY - e.clientY;
      onResize(Math.max(120, Math.min(400, dragRef.current.startH + delta)));
    };
    const onUp = () => { window.removeEventListener('mousemove', onMove); window.removeEventListener('mouseup', onUp); };
    window.addEventListener('mousemove', onMove);
    window.addEventListener('mouseup', onUp);
  }, [height, onResize]);

  return (
    <div style={{ height, background:'#1e1e1e', borderTop:'1px solid #2d2d2d', display:'flex', flexDirection:'column', flexShrink:0 }}>
      {/* Resize handle */}
      <div onMouseDown={handleDragStart} style={{ height:4, cursor:'ns-resize', background:'transparent', flexShrink:0 }}
        onMouseEnter={e=>e.currentTarget.style.background='#2b8ce866'}
        onMouseLeave={e=>e.currentTarget.style.background='transparent'}
      />
      {/* Tab bar */}
      <div style={{ height:28, display:'flex', alignItems:'stretch', borderBottom:'1px solid #2d2d2d', flexShrink:0 }}>
        {BOTTOM_PANELS.map(p=>{
          const active = activePanel===p.id;
          return (
            <button key={p.id} onClick={()=>setActivePanel(p.id)} style={{
              display:'flex', alignItems:'center', gap:6, padding:'0 14px',
              background:active?'#252526':'transparent', border:'none', borderTop:`2px solid ${active?'#2b8ce8':'transparent'}`,
              cursor:'pointer', color:active?'#9a9a9a':'#5a5a5a', fontFamily:'Space Grotesk, sans-serif',
              borderRight:'1px solid #2d2d2d', fontSize:11, fontWeight:active?600:400, transition:'all 0.12s',
            }}
              onMouseEnter={e=>{ if(!active) e.currentTarget.style.background='#2a2d2e'; }}
              onMouseLeave={e=>{ if(!active) e.currentTarget.style.background='transparent'; }}
            >
              <p.Icon size={12} color="currentColor"/>
              {p.label}
            </button>
          );
        })}
        <div style={{ flex:1 }}/>
        <button onClick={onClose} style={{ background:'transparent', border:'none', cursor:'pointer', color:'#454545', padding:'0 10px', display:'flex', alignItems:'center' }}
          onMouseEnter={e=>e.currentTarget.style.color='#9a9a9a'} onMouseLeave={e=>e.currentTarget.style.color='#454545'}
          title="Close panel"
        ><IcoX size={12}/></button>
      </div>
      {/* Content */}
      <div style={{ flex:1, overflow:'hidden' }}>
        {activePanel==='syslog'     && <SystemLogPanel/>}
        {activePanel==='plcmonitor' && <PLCMonitorPanel/>}
      </div>
    </div>
  );
}

/* ─────────────────────────────────────────
   Floating Window
───────────────────────────────────────── */
function FloatingWindow({ id, title, color='#2b8ce8', children, x, y, w, h, onMove, onClose, zIndex, onFocus }) {
  const dragRef = useRef(null);

  const handleTitleDrag = useCallback(e => {
    e.preventDefault();
    dragRef.current = { startX: e.clientX - x, startY: e.clientY - y };
    const onMove_ = e => onMove(id, e.clientX - dragRef.current.startX, e.clientY - dragRef.current.startY);
    const onUp = () => { window.removeEventListener('mousemove', onMove_); window.removeEventListener('mouseup', onUp); };
    window.addEventListener('mousemove', onMove_);
    window.addEventListener('mouseup', onUp);
  }, [x, y, id, onMove]);

  return (
    <div
      onMouseDown={() => onFocus(id)}
      style={{
        position:'fixed', left:x, top:y, width:w, height:h, zIndex,
        background:'#252526', border:`1px solid ${color}44`,
        borderRadius:8, overflow:'hidden', boxShadow:'0 16px 48px rgba(0,0,0,0.7)',
        display:'flex', flexDirection:'column',
      }}
    >
      {/* Title bar */}
      <div
        onMouseDown={handleTitleDrag}
        style={{
          height:32, background:'#1e1e1e', borderBottom:`1px solid ${color}33`,
          display:'flex', alignItems:'center', padding:'0 10px', gap:8,
          cursor:'grab', flexShrink:0, userSelect:'none',
          borderTop:`2px solid ${color}`,
        }}
      >
        <div style={{ width:6, height:6, borderRadius:'50%', background:color }}/>
        <span style={{ flex:1, fontSize:11, fontFamily:'JetBrains Mono, monospace', fontWeight:700, color:'#9a9a9a' }}>{title}</span>
        <div style={{ display:'flex', gap:4 }}>
          <button style={{ width:12, height:12, borderRadius:'50%', background:'#f5a62344', border:'1px solid #f5a62366', cursor:'pointer' }}/>
          <button onClick={()=>onClose(id)} style={{ width:12, height:12, borderRadius:'50%', background:'#e8404044', border:'1px solid #e8404066', cursor:'pointer' }}/>
        </div>
      </div>
      <div style={{ flex:1, overflow:'hidden' }}>{children}</div>
    </div>
  );
}

/* ─────────────────────────────────────────
   Task Picker (for "open tab")
───────────────────────────────────────── */
function TaskPickerDropdown({ allTasks, openTaskIds, onSelect, onClose }) {
  const available = allTasks.filter(t => !openTaskIds.includes(t.id));
  return (
    <div style={{
      position:'absolute', top:36, left:0, zIndex:900,
      background:'#252526', border:'1px solid #3c3c3c', borderRadius:7,
      boxShadow:'0 16px 40px rgba(0,0,0,0.6)', padding:'6px', minWidth:240,
    }}>
      <div style={{ fontSize:9, color:'#454545', fontWeight:700, textTransform:'uppercase', letterSpacing:'0.1em', padding:'4px 8px 6px' }}>Open Task</div>
      {available.length===0 ? (
        <div style={{ padding:'12px 8px', color:'#454545', fontSize:11, textAlign:'center' }}>All tasks already open</div>
      ) : available.map(t=>{
        const color = DOCK_TASK_COLORS[t.type]||'#9a9a9a';
        return (
          <div key={t.id} onClick={()=>{ onSelect(t.id); onClose(); }} style={{
            display:'flex', alignItems:'center', gap:8, padding:'7px 10px', borderRadius:5, cursor:'pointer',
          }}
            onMouseEnter={e=>e.currentTarget.style.background='#2a2d2e'}
            onMouseLeave={e=>e.currentTarget.style.background='transparent'}
          >
            <span style={{ fontSize:8, fontWeight:800, color, background:`${color}18`, border:`1px solid ${color}44`, borderRadius:3, padding:'1px 5px', flexShrink:0 }}>
              {DOCK_TASK_CHIPS[t.type]||'?'}
            </span>
            <span style={{ fontSize:12, fontFamily:'JetBrains Mono, monospace', color:'#bfbfbf' }}>{t.name}</span>
            <span style={{ marginLeft:'auto', fontSize:9, color:'#454545' }}>{t.devices.length}d</span>
          </div>
        );
      })}
      <div style={{ borderTop:'1px solid #2d2d2d', marginTop:4, paddingTop:4 }}>
        <div onClick={onClose} style={{ padding:'5px 10px', fontSize:11, color:'#454545', cursor:'pointer', borderRadius:4 }}
          onMouseEnter={e=>e.currentTarget.style.background='#2a2d2e'} onMouseLeave={e=>e.currentTarget.style.background='transparent'}
        >Cancel</div>
      </div>
    </div>
  );
}

/* ─────────────────────────────────────────
   DockWorkspace (main export)
───────────────────────────────────────── */
function DockWorkspace({
  projects, allTasks,
  openTaskIds, activeTaskId, activeDeviceId,
  onSelectTask, onCloseTab, onOpenTab,
  setActiveDeviceId,
  onAddDevice, onMoveDevice, onDeleteDevice, onToggleDeviceConnect,
}) {
  const [bottomH, setBottomH] = useState(180);
  const [showBottom, setShowBottom] = useState(true);
  const [showPicker, setShowPicker] = useState(false);
  const [floats, setFloats] = useState([
    { id:'cam-float', title:'Camera Feed — Basler_cam_01', color:'#2b8ce8', x:320, y:120, w:520, h:380, visible:false },
    { id:'pat-float', title:'Pattern Library', color:'#f5a623', x:180, y:80,  w:420, h:500, visible:false },
  ]);
  const [topFloat, setTopFloat] = useState(null);

  const openTasks = allTasks.filter(t => openTaskIds.includes(t.id));
  const activeTask = allTasks.find(t => t.id === activeTaskId);

  const moveFloat = useCallback((id, x, y) => {
    setFloats(prev => prev.map(f => f.id===id ? {...f, x, y} : f));
  }, []);

  const closeFloat = useCallback(id => {
    setFloats(prev => prev.map(f => f.id===id ? {...f, visible:false} : f));
  }, []);

  const floatZIndex = id => topFloat===id ? 202 : 200;

  return (
    <div style={{ flex:1, display:'flex', flexDirection:'column', overflow:'hidden', position:'relative' }}>
      {/* Tab bar */}
      <div style={{ position:'relative' }}>
        <TaskTabBar
          openTasks={openTasks}
          activeTaskId={activeTaskId}
          onSelect={onSelectTask}
          onClose={onCloseTab}
          onAddTab={() => setShowPicker(p=>!p)}
        />
        {showPicker && (
          <TaskPickerDropdown
            allTasks={allTasks}
            openTaskIds={openTaskIds}
            onSelect={onOpenTab}
            onClose={() => setShowPicker(false)}
          />
        )}
      </div>

      {/* Empty state when no tabs open */}
      {openTasks.length === 0 ? (
        <div style={{ flex:1, display:'flex', flexDirection:'column', alignItems:'center', justifyContent:'center', color:'#454545', gap:12 }}>
          <IcoTask size={40} color="#2d2d2d"/>
          <div style={{ fontSize:14, fontWeight:700, color:'#454545' }}>No tasks open</div>
          <button onClick={()=>setShowPicker(true)} style={{ background:'transparent', border:'1px solid #3c3c3c', borderRadius:6, padding:'8px 20px', fontSize:12, color:'#5a5a5a', cursor:'pointer' }}
            onMouseEnter={e=>e.currentTarget.style.borderColor='#2b8ce8'} onMouseLeave={e=>e.currentTarget.style.borderColor='#3c3c3c'}
          >Open a task</button>
        </div>
      ) : activeTask ? (
        <div style={{ flex:1, display:'flex', flexDirection:'column', overflow:'hidden' }}>
          {/* Main workspace */}
          <div style={{ flex:1, overflow:'hidden' }}>
            <TaskWorkspace
              task={activeTask}
              allTasks={allTasks}
              activeDeviceId={activeDeviceId}
              setActiveDeviceId={setActiveDeviceId}
              onAddDevice={onAddDevice}
              onMoveDevice={onMoveDevice}
              onDeleteDevice={onDeleteDevice}
              onToggleDeviceConnect={onToggleDeviceConnect}
            />
          </div>

          {/* Bottom dock */}
          {showBottom ? (
            <BottomDock height={bottomH} onResize={setBottomH} onClose={()=>setShowBottom(false)}/>
          ) : (
            <div style={{ height:24, background:'#1e1e1e', borderTop:'1px solid #2d2d2d', display:'flex', alignItems:'center', padding:'0 10px', gap:8 }}>
              <button onClick={()=>setShowBottom(true)} style={{ background:'transparent', border:'none', cursor:'pointer', color:'#454545', fontSize:10, fontWeight:700, display:'flex', alignItems:'center', gap:6 }}
                onMouseEnter={e=>e.currentTarget.style.color='#2b8ce8'} onMouseLeave={e=>e.currentTarget.style.color='#454545'}
              >
                <IcoLog size={11}/> System Log
                <IcoPLC size={11}/> PLC Monitor
              </button>
            </div>
          )}
        </div>
      ) : null}

      {/* Floating windows */}
      {floats.filter(f=>f.visible).map(f=>(
        <FloatingWindow
          key={f.id} {...f}
          onMove={moveFloat}
          onClose={closeFloat}
          onFocus={setTopFloat}
          zIndex={floatZIndex(f.id)}
        >
          {f.id==='cam-float'  && <CameraPanel/>}
          {f.id==='pat-float'  && <VisionPanel/>}
        </FloatingWindow>
      ))}

      {/* Click backdrop to close picker */}
      {showPicker && <div onClick={()=>setShowPicker(false)} style={{ position:'fixed', inset:0, zIndex:800 }}/>}
    </div>
  );
}

Object.assign(window, { DockWorkspace });
