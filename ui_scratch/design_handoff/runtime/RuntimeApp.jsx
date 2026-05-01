
// RuntimeApp.jsx - NCR Vision Runtime — Field Operator UI
const { useState, useEffect, useRef, useCallback } = React;

/* ─── CSS Variables (theme) injected via JS ─── */
const THEMES = {
  dark: {
    '--bg':        '#060d18', '--surface':   '#0b1524', '--surface2': '#0f1d2e',
    '--border':    '#1a2a3a', '--border2':   '#243444',
    '--text':      '#dce8f5', '--text2':     '#4a6080',  '--text3': '#1f2d42',
    '--accent':    '#2b8ce8', '--accent-bg': '#2b8ce815',
    '--ok':        '#22d17a', '--ok-bg':     '#22d17a15',
    '--warn':      '#f5a623', '--warn-bg':   '#f5a62315',
    '--err':       '#e84040', '--err-bg':    '#e8404015',
    '--card-bg':   '#0b1524', '--card-header':'#060d18',
    '--topbar':    '#040a12',
  },
  light: {
    '--bg':        '#eef2f8', '--surface':   '#ffffff',  '--surface2': '#f4f7fb',
    '--border':    '#d0dae8', '--border2':   '#b8c8da',
    '--text':      '#1a2840', '--text2':     '#4a6080',  '--text3': '#8aa0b8',
    '--accent':    '#1a6bbf', '--accent-bg': '#1a6bbf12',
    '--ok':        '#16a34a', '--ok-bg':     '#16a34a12',
    '--warn':      '#d97706', '--warn-bg':   '#d9770612',
    '--err':       '#dc2626', '--err-bg':    '#dc262612',
    '--card-bg':   '#ffffff', '--card-header':'#f4f7fb',
    '--topbar':    '#1a2840',
  },
};

function applyTheme(name) {
  const vars = THEMES[name];
  Object.entries(vars).forEach(([k,v]) => document.documentElement.style.setProperty(k,v));
}

/* ─── Task data ─── */
const TASK_DEFS = [
  { id:1, name:'Task_Loc_01',  type:'LocalizationTask', line:'Line A', devices:[{type:'camera',name:'Basler_cam_01',ok:true},{type:'plc',name:'PLC_Mitsu_01',ok:true},{type:'robot',name:'Robot_Fanuc_01',ok:true}] },
  { id:2, name:'Task_Pick_01', type:'PickPlaceTask',     line:'Line A', devices:[{type:'camera',name:'Basler_cam_02',ok:false},{type:'plc',name:'PLC_Mitsu_02',ok:true},{type:'robot',name:'Robot_Fanuc_02',ok:true}] },
  { id:3, name:'Inspect_Top',  type:'InspectTask',       line:'Line B', devices:[{type:'camera',name:'Basler_cam_03',ok:true},{type:'plc',name:'PLC_Mitsu_03',ok:true}] },
  { id:4, name:'Task_Loc_02',  type:'LocalizationTask',  line:'Line C', devices:[{type:'camera',name:'Basler_cam_04',ok:true},{type:'plc',name:'PLC_Mitsu_04',ok:true}] },
];

const TASK_COLORS = { LocalizationTask:'#2b8ce8', PickPlaceTask:'#22d17a', InspectTask:'#f5a623' };
const TASK_CHIPS  = { LocalizationTask:'LOC', PickPlaceTask:'PICK', InspectTask:'INSP' };
const DEV_COLORS  = { camera:'#2b8ce8', plc:'#22d17a', robot:'#f5a623' };

/* ─── Sparkline ─── */
function Sparkline({ data, color, width=120, height=32 }) {
  if (!data.length) return null;
  const max = Math.max(...data, 1);
  const min = Math.min(...data, 0);
  const range = max - min || 1;
  const pts = data.map((v,i) => {
    const x = (i/(data.length-1)) * width;
    const y = height - ((v-min)/range) * height * 0.85 - 2;
    return `${x},${y}`;
  }).join(' ');
  const fill = `M 0,${height} L ${data.map((v,i)=>`${(i/(data.length-1))*width},${height-((v-min)/range)*height*0.85-2}`).join(' L ')} L ${width},${height} Z`;
  return (
    <svg width={width} height={height} style={{ overflow:'visible' }}>
      <defs>
        <linearGradient id={`sg${color.replace('#','')}`} x1="0" y1="0" x2="0" y2="1">
          <stop offset="0%" stopColor={color} stopOpacity="0.25"/>
          <stop offset="100%" stopColor={color} stopOpacity="0.02"/>
        </linearGradient>
      </defs>
      <path d={fill} fill={`url(#sg${color.replace('#','')})`}/>
      <polyline points={pts} fill="none" stroke={color} strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round"/>
    </svg>
  );
}

/* ─── Device Pill ─── */
function DevPill({ type, name, ok }) {
  const color = ok ? DEV_COLORS[type] : 'var(--text3)';
  const labels = { camera:'CAM', plc:'PLC', robot:'BOT' };
  return (
    <div style={{ display:'flex', alignItems:'center', gap:4, padding:'3px 8px', borderRadius:4, background: ok ? `${DEV_COLORS[type]}15` : 'var(--surface2)', border:`1px solid ${ok ? DEV_COLORS[type]+'33' : 'var(--border)'}` }} title={name}>
      <div style={{ width:6, height:6, borderRadius:'50%', background: ok ? color : 'var(--text3)', boxShadow: ok ? `0 0 5px ${color}` : 'none' }}/>
      <span style={{ fontSize:10, fontWeight:700, color: ok ? color : 'var(--text3)', letterSpacing:'0.05em', fontFamily:'JetBrains Mono, monospace' }}>{labels[type]||type}</span>
    </div>
  );
}

/* ─── Stat Cell ─── */
function StatCell({ label, value, unit, color='var(--text)', size=22 }) {
  return (
    <div style={{ flex:1, display:'flex', flexDirection:'column', alignItems:'center', padding:'8px 4px', borderRight:'1px solid var(--border)' }}>
      <span style={{ fontSize:size, fontWeight:800, color, fontFamily:'JetBrains Mono, monospace', lineHeight:1.1 }}>{value}</span>
      {unit && <span style={{ fontSize:9, color:'var(--text3)', marginTop:1 }}>{unit}</span>}
      <span style={{ fontSize:9, color:'var(--text2)', marginTop:2, textTransform:'uppercase', letterSpacing:'0.07em', fontWeight:600 }}>{label}</span>
    </div>
  );
}

/* ─── Action Button ─── */
function ActionBtn({ label, color='var(--accent)', bg='var(--accent-bg)', border='var(--accent)', onClick, icon, disabled }) {
  return (
    <button onClick={onClick} disabled={disabled} style={{
      display:'flex', alignItems:'center', gap:5, padding:'7px 14px',
      background: disabled ? 'var(--surface2)' : bg,
      border:`1px solid ${disabled ? 'var(--border)' : border}`,
      borderRadius:5, cursor: disabled ? 'default' : 'pointer',
      color: disabled ? 'var(--text3)' : color, fontWeight:600, fontSize:12,
      fontFamily:'Space Grotesk, sans-serif', transition:'all 0.14s',
    }}
      onMouseEnter={e=>{ if(!disabled){ e.currentTarget.style.background=color; e.currentTarget.style.color='#fff'; e.currentTarget.style.borderColor=color; }}}
      onMouseLeave={e=>{ if(!disabled){ e.currentTarget.style.background=bg; e.currentTarget.style.color=color; e.currentTarget.style.borderColor=border; }}}
    >
      {icon}{label}
    </button>
  );
}

/* ─── Task Monitor Card ─── */
function TaskMonitorCard({ task, onFloat, onFullscreen, compact=false }) {
  const [state, setState] = useState('running'); // running | stopped | error
  const [runs, setRuns]   = useState(2847);
  const [okRate, setOkRate] = useState(96.2);
  const [cycleMs, setCycleMs] = useState(124);
  const [score, setScore] = useState(0.967);
  const [lastResult, setLastResult] = useState({ ok:true, score:0.967, x:823, y:512, angle:12.4 });
  const [history, setHistory] = useState(Array.from({length:20},()=>90+Math.random()*8));
  const [triggering, setTriggering] = useState(false);

  const color = TASK_COLORS[task.type]||'var(--accent)';
  const chip  = TASK_CHIPS[task.type] || '?';

  /* Simulate live data when running */
  useEffect(()=>{
    if(state!=='running') return;
    const t = setInterval(()=>{
      const ok = Math.random() > 0.04;
      const sc = ok ? 0.88+Math.random()*0.12 : 0.45+Math.random()*0.3;
      setRuns(r=>r+1);
      setScore(parseFloat(sc.toFixed(3)));
      setCycleMs(Math.floor(100+Math.random()*80));
      setOkRate(r=>parseFloat(Math.max(85,Math.min(99.9,r+(ok?0.01:-0.1))).toFixed(1)));
      setLastResult({ ok, score:sc, x:800+Math.floor(Math.random()*50), y:500+Math.floor(Math.random()*30), angle:parseFloat((10+Math.random()*10).toFixed(1)) });
      setHistory(h=>[...h.slice(1), ok?95+Math.random()*4:75+Math.random()*10]);
    }, 2200);
    return ()=>clearInterval(t);
  }, [state]);

  const statusColor = state==='running' ? 'var(--ok)' : state==='error' ? 'var(--err)' : 'var(--text3)';
  const statusLabel = state==='running' ? 'RUNNING' : state==='error' ? 'ERROR' : 'STOPPED';
  const borderColor = state==='running' ? 'var(--ok)' : state==='error' ? 'var(--err)' : 'var(--border)';

  const handleTrigger = () => {
    setTriggering(true);
    setTimeout(()=>setTriggering(false), 800);
  };

  return (
    <div style={{
      background:'var(--card-bg)', border:`1px solid ${borderColor}`,
      borderRadius:10, display:'flex', flexDirection:'column', overflow:'hidden',
      boxShadow: state==='error' ? `0 0 20px var(--err)33` : '0 2px 12px rgba(0,0,0,0.3)',
      transition:'border-color 0.3s, box-shadow 0.3s',
      height:'100%',
    }}>
      {/* Card header */}
      <div style={{ background:'var(--card-header)', padding: compact?'8px 12px':'10px 14px', display:'flex', alignItems:'center', gap:8, borderBottom:'1px solid var(--border)', flexShrink:0 }}>
        <span style={{ fontSize:9, fontWeight:800, color, background:`${color}18`, border:`1px solid ${color}44`, borderRadius:3, padding:'2px 6px', letterSpacing:'0.06em', flexShrink:0 }}>{chip}</span>
        <div style={{ flex:1, minWidth:0 }}>
          <div style={{ fontSize: compact?12:14, fontWeight:700, color:'var(--text)', fontFamily:'JetBrains Mono, monospace', overflow:'hidden', textOverflow:'ellipsis', whiteSpace:'nowrap' }}>{task.name}</div>
          <div style={{ fontSize:10, color:'var(--text2)' }}>{task.line}</div>
        </div>
        {/* Status badge */}
        <div style={{ display:'flex', alignItems:'center', gap:5, padding:'3px 10px', borderRadius:5, background:`${statusColor}18`, border:`1px solid ${statusColor}44` }}>
          <div style={{ width:7, height:7, borderRadius:'50%', background:statusColor, boxShadow: state==='running'?`0 0 6px ${statusColor}`:'' }}/>
          <span style={{ fontSize:10, fontWeight:800, color:statusColor, letterSpacing:'0.06em' }}>{statusLabel}</span>
        </div>
        {/* Window controls */}
        <div style={{ display:'flex', gap:3, flexShrink:0 }}>
          {onFloat && <button onClick={onFloat} title="Float window" style={{ background:'transparent', border:'none', cursor:'pointer', color:'var(--text3)', padding:'3px', borderRadius:4, display:'flex' }}
            onMouseEnter={e=>e.currentTarget.style.color='var(--accent)'} onMouseLeave={e=>e.currentTarget.style.color='var(--text3)'}
          >
            <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round"><rect x="3" y="3" width="18" height="18" rx="2"/><path d="M9 3v18M15 3v18M3 9h18M3 15h18" strokeOpacity="0.5"/></svg>
          </button>}
          {onFullscreen && <button onClick={onFullscreen} title="Fullscreen" style={{ background:'transparent', border:'none', cursor:'pointer', color:'var(--text3)', padding:'3px', borderRadius:4, display:'flex' }}
            onMouseEnter={e=>e.currentTarget.style.color='var(--accent)'} onMouseLeave={e=>e.currentTarget.style.color='var(--text3)'}
          >
            <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round"><path d="M8 3H5a2 2 0 0 0-2 2v3m18 0V5a2 2 0 0 0-2-2h-3m0 18h3a2 2 0 0 0 2-2v-3M3 16v3a2 2 0 0 0 2 2h3"/></svg>
          </button>}
        </div>
      </div>

      {/* Device status pills */}
      <div style={{ padding:'8px 12px', borderBottom:'1px solid var(--border)', display:'flex', gap:5, flexWrap:'wrap', flexShrink:0, background:'var(--card-header)' }}>
        {task.devices.map((d,i)=><DevPill key={i} {...d}/>)}
      </div>

      {/* Stats row */}
      <div style={{ display:'flex', borderBottom:'1px solid var(--border)', flexShrink:0 }}>
        <StatCell label="Total Runs" value={runs.toLocaleString()} color="var(--accent)" size={compact?18:22}/>
        <StatCell label="OK Rate" value={`${okRate}%`} color={okRate>=95?'var(--ok)':okRate>=90?'var(--warn)':'var(--err)'} size={compact?18:22}/>
        <StatCell label="Cycle" value={cycleMs} unit="ms" color="var(--text)" size={compact?18:22}/>
        <StatCell label="Score" value={score.toFixed(3)} color={score>=0.9?'var(--ok)':score>=0.75?'var(--warn)':'var(--err)'} size={compact?18:22}/>
      </div>

      {/* Camera feed + last result */}
      {!compact && (
        <div style={{ flex:1, position:'relative', background:'#030810', overflow:'hidden', minHeight:100 }}>
          {/* Grid bg */}
          <svg width="100%" height="100%" style={{ position:'absolute', inset:0, opacity:0.4 }}>
            {Array.from({length:12},(_,i)=><line key={`h${i}`} x1="0" y1={`${i*8.33}%`} x2="100%" y2={`${i*8.33}%`} stroke="#1a2a3a" strokeWidth="0.5"/>)}
            {Array.from({length:16},(_,i)=><line key={`v${i}`} x1={`${i*6.25}%`} y1="0" x2={`${i*6.25}%`} y2="100%" stroke="#1a2a3a" strokeWidth="0.5"/>)}
          </svg>
          {/* Corner brackets */}
          {[['12px','12px','borderTop','borderLeft'],['12px','right:12px','borderTop','borderRight'],['bottom:12px','12px','borderBottom','borderLeft'],['bottom:12px','right:12px','borderBottom','borderRight']].map(([t,l,b1,b2],i)=>(
            <div key={i} style={{ position:'absolute', [t.includes('bottom')?'bottom':'top']:12, [l.includes('right')?'right':'left']:12, width:16, height:16, [b1]:`1.5px solid ${color}`, [b2]:`1.5px solid ${color}` }}/>
          ))}
          {/* Reticle */}
          <div style={{ position:'absolute', inset:0, display:'flex', alignItems:'center', justifyContent:'center' }}>
            <svg width="60" height="44">
              <rect x="4" y="4" width="52" height="36" rx="1" fill="none" stroke={color} strokeWidth="1" strokeDasharray="6 3" opacity="0.5"/>
              <line x1="30" y1="0" x2="30" y2="44" stroke={`${color}44`} strokeWidth="0.8"/>
              <line x1="0" y1="22" x2="60" y2="22" stroke={`${color}44`} strokeWidth="0.8"/>
              <circle cx="30" cy="22" r="3.5" fill="none" stroke={color} strokeWidth="1.5" opacity="0.7"/>
              <circle cx="30" cy="22" r="1" fill={color}/>
            </svg>
          </div>
          {/* Match result overlay */}
          <div style={{ position:'absolute', bottom:8, left:10, right:10, display:'flex', justifyContent:'space-between', alignItems:'center' }}>
            <span style={{ fontSize:9, color:'#1f3050', fontFamily:'JetBrains Mono, monospace' }}>Camera Feed Placeholder</span>
            <div style={{ display:'flex', alignItems:'center', gap:5, background:'rgba(6,13,24,0.85)', padding:'3px 8px', borderRadius:4, border:`1px solid ${lastResult.ok?'var(--ok)':'var(--err)'}44` }}>
              <div style={{ width:5, height:5, borderRadius:'50%', background:lastResult.ok?'var(--ok)':'var(--err)' }}/>
              <span style={{ fontSize:9, fontFamily:'JetBrains Mono, monospace', color:lastResult.ok?'var(--ok)':'var(--err)', fontWeight:700 }}>
                {lastResult.ok?'OK':'NG'} {lastResult.score.toFixed(3)} · ({lastResult.x},{lastResult.y}) · {lastResult.angle}°
              </span>
            </div>
          </div>
          {/* LIVE indicator */}
          <div style={{ position:'absolute', top:10, right:12, fontSize:9, color:state==='running'?'var(--ok)':'var(--text3)', fontFamily:'JetBrains Mono, monospace', fontWeight:700 }}>
            {state==='running'?'● LIVE':'○ PAUSED'}
          </div>
        </div>
      )}

      {/* Trend sparkline */}
      {!compact && (
        <div style={{ padding:'6px 12px 4px', borderTop:'1px solid var(--border)', display:'flex', alignItems:'center', gap:10, flexShrink:0 }}>
          <span style={{ fontSize:9, color:'var(--text3)', fontWeight:700, textTransform:'uppercase', letterSpacing:'0.07em', flexShrink:0 }}>OK Rate trend</span>
          <div style={{ flex:1 }}><Sparkline data={history} color={okRate>=90?'var(--ok)':'var(--warn)'} width={180} height={28}/></div>
          <span style={{ fontSize:10, fontFamily:'JetBrains Mono, monospace', color:'var(--text2)', flexShrink:0 }}>{okRate}%</span>
        </div>
      )}

      {/* Action bar */}
      <div style={{ padding:'8px 12px', borderTop:'1px solid var(--border)', display:'flex', gap:6, flexWrap:'wrap', flexShrink:0 }}>
        {state==='running' ? (
          <ActionBtn label="STOP" color="var(--err)" bg="var(--err-bg)" border="var(--err)" onClick={()=>setState('stopped')}
            icon={<svg width="10" height="10" viewBox="0 0 24 24" fill="currentColor"><rect x="3" y="3" width="18" height="18" rx="2"/></svg>}/>
        ) : (
          <ActionBtn label="START" color="var(--ok)" bg="var(--ok-bg)" border="var(--ok)" onClick={()=>setState('running')}
            icon={<svg width="10" height="10" viewBox="0 0 24 24" fill="currentColor"><polygon points="5 3 19 12 5 21"/></svg>}/>
        )}
        <ActionBtn label="RESET" color="var(--warn)" bg="var(--warn-bg)" border="var(--warn)" onClick={()=>{ setState('stopped'); setRuns(0); setOkRate(100); }}
          icon={<svg width="10" height="10" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5" strokeLinecap="round"><polyline points="1 4 1 10 7 10"/><path d="M3.51 15a9 9 0 1 0 .49-5.26"/></svg>}/>
        <ActionBtn label={triggering?'WAIT…':'TRIGGER'} color="var(--accent)" bg="var(--accent-bg)" border="var(--accent)" disabled={state!=='running'||triggering} onClick={handleTrigger}
          icon={<svg width="10" height="10" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5" strokeLinecap="round"><polygon points="13 2 3 14 12 14 11 22 21 10 12 10 13 2"/></svg>}/>
        {state==='error' && (
          <ActionBtn label="ACK" color="var(--err)" bg="var(--err-bg)" border="var(--err)" onClick={()=>setState('stopped')}/>
        )}
      </div>
    </div>
  );
}

/* ─── Alarm Bar ─── */
const INIT_ALARMS = [
  { id:1, time:'09:44:10', task:'Task_Pick_01', msg:'Camera Basler_cam_02 disconnected', level:'error' },
  { id:2, time:'09:43:18', task:'Task_Loc_01',  msg:'Low match score (0.521) — threshold 0.75', level:'warn'  },
];

function AlarmBar({ alarms, onAck, onAckAll }) {
  if (!alarms.length) return null;
  const hasErr = alarms.some(a=>a.level==='error');
  const bg     = hasErr ? 'var(--err-bg)' : 'var(--warn-bg)';
  const border = hasErr ? 'var(--err)' : 'var(--warn)';
  return (
    <div style={{ background:bg, borderBottom:`1px solid ${border}44`, padding:'6px 16px', display:'flex', alignItems:'center', gap:12, flexShrink:0 }}>
      <div style={{ display:'flex', alignItems:'center', gap:5, flexShrink:0 }}>
        <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke={hasErr?'var(--err)':'var(--warn)'} strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
          <path d="M10.29 3.86L1.82 18a2 2 0 0 0 1.71 3h16.94a2 2 0 0 0 1.71-3L13.71 3.86a2 2 0 0 0-3.42 0z"/>
          <line x1="12" y1="9" x2="12" y2="13"/><line x1="12" y1="17" x2="12.01" y2="17"/>
        </svg>
        <span style={{ fontSize:10, fontWeight:800, color:hasErr?'var(--err)':'var(--warn)', letterSpacing:'0.06em' }}>{alarms.length} ALARM{alarms.length!==1?'S':''}</span>
      </div>
      <div style={{ flex:1, display:'flex', gap:12, overflow:'hidden' }}>
        {alarms.slice(0,3).map(a=>(
          <div key={a.id} style={{ display:'flex', alignItems:'center', gap:6, padding:'2px 10px', borderRadius:4, background:'rgba(0,0,0,0.15)', border:`1px solid ${a.level==='error'?'var(--err)':'var(--warn)'}44`, flexShrink:0 }}>
            <span style={{ fontSize:9, color:'var(--text2)', fontFamily:'JetBrains Mono, monospace' }}>{a.time}</span>
            <span style={{ fontSize:9, fontWeight:700, color:'var(--text2)' }}>[{a.task}]</span>
            <span style={{ fontSize:11, color:'var(--text)', fontWeight:500 }}>{a.msg}</span>
            <button onClick={()=>onAck(a.id)} style={{ background:'transparent', border:'none', cursor:'pointer', color:'var(--text3)', padding:'1px', display:'flex', borderRadius:3 }}
              onMouseEnter={e=>e.currentTarget.style.color='var(--err)'} onMouseLeave={e=>e.currentTarget.style.color='var(--text3)'}
            ><svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.2" strokeLinecap="round"><line x1="18" y1="6" x2="6" y2="18"/><line x1="6" y1="6" x2="18" y2="18"/></svg></button>
          </div>
        ))}
      </div>
      <button onClick={onAckAll} style={{ background:'transparent', border:'1px solid var(--border2)', borderRadius:4, padding:'4px 10px', fontSize:10, fontWeight:700, color:'var(--text2)', cursor:'pointer', flexShrink:0, fontFamily:'Space Grotesk, sans-serif' }}>
        ACK ALL
      </button>
    </div>
  );
}

/* ─── Floating Task Window ─── */
function FloatingTaskCard({ task, x, y, onMove, onClose, onDock, zIndex, onFocus }) {
  const dragRef = useRef(null);
  const handleDrag = useCallback(e => {
    e.preventDefault();
    dragRef.current = { sx: e.clientX - x, sy: e.clientY - y };
    const mv = e => onMove(task.id, e.clientX - dragRef.current.sx, e.clientY - dragRef.current.sy);
    const up = () => { window.removeEventListener('mousemove', mv); window.removeEventListener('mouseup', up); };
    window.addEventListener('mousemove', mv);
    window.addEventListener('mouseup', up);
  }, [x, y, task.id, onMove]);
  const color = TASK_COLORS[task.type]||'var(--accent)';
  return (
    <div onMouseDown={()=>onFocus(task.id)} style={{ position:'fixed', left:x, top:y, width:420, height:520, zIndex, display:'flex', flexDirection:'column', borderRadius:10, overflow:'hidden', boxShadow:'0 20px 60px rgba(0,0,0,0.7)', border:`1px solid ${color}44` }}>
      {/* Drag titlebar */}
      <div onMouseDown={handleDrag} style={{ height:32, background:color, display:'flex', alignItems:'center', padding:'0 10px', gap:8, cursor:'grab', userSelect:'none', flexShrink:0 }}>
        <span style={{ fontSize:11, fontFamily:'JetBrains Mono, monospace', fontWeight:700, color:'#fff', flex:1 }}>{task.name} — floating</span>
        <button onClick={()=>onDock(task.id)} title="Dock back to grid" style={{ background:'rgba(255,255,255,0.2)', border:'none', cursor:'pointer', color:'#fff', padding:'3px 8px', borderRadius:4, fontSize:10, fontWeight:700 }}>DOCK</button>
        <button onClick={()=>onClose(task.id)} style={{ background:'rgba(255,255,255,0.2)', border:'none', cursor:'pointer', color:'#fff', width:22, height:22, borderRadius:'50%', display:'flex', alignItems:'center', justifyContent:'center' }}>
          <svg width="10" height="10" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5" strokeLinecap="round"><line x1="18" y1="6" x2="6" y2="18"/><line x1="6" y1="6" x2="18" y2="18"/></svg>
        </button>
      </div>
      <TaskMonitorCard task={task} compact={false}/>
    </div>
  );
}

/* ─── Grid Layout ─── */
function TaskGrid({ tasks, onFloat, onFullscreen }) {
  const count = tasks.length;
  const gridStyle = count === 1
    ? { gridTemplateColumns:'1fr', gridTemplateRows:'1fr' }
    : count === 2
    ? { gridTemplateColumns:'1fr 1fr', gridTemplateRows:'1fr' }
    : count === 3
    ? { gridTemplateColumns:'1fr 1fr', gridTemplateRows:'1fr 1fr' }
    : { gridTemplateColumns:'1fr 1fr', gridTemplateRows:'1fr 1fr' };

  return (
    <div style={{ flex:1, display:'grid', ...gridStyle, gap:8, padding:8, overflow:'hidden', minHeight:0 }}>
      {tasks.map(task => (
        <TaskMonitorCard key={task.id} task={task}
          onFloat={() => onFloat(task.id)}
          onFullscreen={() => onFullscreen(task.id)}
          compact={count >= 4}
        />
      ))}
      {/* Empty slot for 3-task grid */}
      {count === 3 && (
        <div style={{ background:'var(--surface)', border:'1px dashed var(--border)', borderRadius:10, display:'flex', alignItems:'center', justifyContent:'center', color:'var(--text3)', fontSize:13, cursor:'default' }}>
          <div style={{ textAlign:'center' }}>
            <svg width="32" height="32" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1" strokeLinecap="round" style={{ marginBottom:8, opacity:0.3 }}>
              <line x1="12" y1="5" x2="12" y2="19"/><line x1="5" y1="12" x2="19" y2="12"/>
            </svg>
            <div>Add task panel</div>
          </div>
        </div>
      )}
    </div>
  );
}

/* ─── Bottom Event Log Strip ─── */
const EVENT_LOGS = [
  '09:44:10 [WARN]  Task_Pick_01 — Camera disconnected',
  '09:43:19 [OK]    Task_Loc_01 — Match score 0.882 accepted (retry #1)',
  '09:43:18 [WARN]  Task_Loc_01 — Low score 0.521 on pattern #3',
  '09:42:02 [OK]    Task_Loc_01 — Output → D100:8230 D101:5120 D102:124',
  '09:42:01 [OK]    Task_Loc_01 — Match OK score:0.967 pos:(823,512) angle:12.4°',
];
const LOG_COLORS_RT = { OK:'var(--ok)', WARN:'var(--warn)', ERROR:'var(--err)', INFO:'var(--accent)' };

function EventLogStrip({ height, onResize }) {
  const [open, setOpen] = useState(true);
  const dragRef = useRef(null);
  const handleDrag = useCallback(e => {
    dragRef.current = { sy: e.clientY, sh: height };
    const mv = e => onResize(Math.max(60, Math.min(300, dragRef.current.sh + dragRef.current.sy - e.clientY)));
    const up = () => { window.removeEventListener('mousemove',mv); window.removeEventListener('mouseup',up); };
    window.addEventListener('mousemove', mv);
    window.addEventListener('mouseup', up);
  }, [height, onResize]);
  if(!open) return (
    <div style={{ height:28, background:'var(--card-header)', borderTop:'1px solid var(--border)', display:'flex', alignItems:'center', padding:'0 14px', cursor:'pointer' }} onClick={()=>setOpen(true)}>
      <span style={{ fontSize:9, color:'var(--text3)', fontWeight:700, textTransform:'uppercase', letterSpacing:'0.1em' }}>▲ Event Log</span>
    </div>
  );
  return (
    <div style={{ height, background:'var(--card-header)', borderTop:'1px solid var(--border)', display:'flex', flexDirection:'column', flexShrink:0 }}>
      <div onMouseDown={handleDrag} style={{ height:4, cursor:'ns-resize' }} onMouseEnter={e=>e.currentTarget.style.background='var(--accent)44'} onMouseLeave={e=>e.currentTarget.style.background='transparent'}/>
      <div style={{ padding:'4px 14px 4px', display:'flex', alignItems:'center', gap:8, flexShrink:0 }}>
        <span style={{ fontSize:9, fontWeight:700, color:'var(--text3)', textTransform:'uppercase', letterSpacing:'0.1em', flex:1 }}>Event Log</span>
        <button onClick={()=>setOpen(false)} style={{ background:'transparent', border:'none', cursor:'pointer', color:'var(--text3)', padding:'2px', borderRadius:3, display:'flex', fontSize:9, fontWeight:700 }}>▼</button>
      </div>
      <div style={{ flex:1, overflowY:'auto', padding:'2px 0' }}>
        {EVENT_LOGS.map((line,i)=>{
          const match = line.match(/\[(OK|WARN|ERROR|INFO)\]/);
          const level = match?.[1]||'INFO';
          return (
            <div key={i} style={{ padding:'2px 14px', fontFamily:'JetBrains Mono, monospace', fontSize:10, color:'var(--text2)', display:'flex', gap:8 }}>
              <span style={{ color:LOG_COLORS_RT[level]||'var(--text2)', fontWeight:700, flexShrink:0 }}>[{level}]</span>
              <span>{line.replace(/\s*\[(OK|WARN|ERROR|INFO)\]\s*/,'')}</span>
            </div>
          );
        })}
      </div>
    </div>
  );
}

/* ─── Top Bar ─── */
function RuntimeTopBar({ theme, onToggleTheme, taskCount, connectedCount, onAddTask }) {
  const [time, setTime] = useState(new Date().toLocaleTimeString());
  useEffect(()=>{ const t=setInterval(()=>setTime(new Date().toLocaleTimeString()),1000); return()=>clearInterval(t); },[]);
  return (
    <div style={{ height:48, background:'var(--topbar)', display:'flex', alignItems:'center', padding:'0 16px', gap:12, flexShrink:0, borderBottom:'2px solid var(--accent)' }}>
      {/* Logo */}
      <div style={{ display:'flex', alignItems:'center', gap:8, paddingRight:16, borderRight:'1px solid #ffffff22' }}>
        <svg width="22" height="22" viewBox="0 0 24 24" fill="none">
          <rect x="3" y="3" width="8" height="8" rx="1.5" fill="var(--accent)"/>
          <rect x="13" y="3" width="8" height="8" rx="1.5" fill="#ffffff44"/>
          <rect x="3" y="13" width="8" height="8" rx="1.5" fill="#ffffff44"/>
          <circle cx="17" cy="17" r="4" fill="none" stroke="var(--accent)" strokeWidth="1.5"/>
          <circle cx="17" cy="17" r="1.5" fill="var(--accent)"/>
        </svg>
        <div>
          <div style={{ fontSize:13, fontWeight:800, color:'#fff', letterSpacing:'-0.01em', lineHeight:1 }}>NCR<span style={{ color:'var(--accent)' }}>vision</span></div>
          <div style={{ fontSize:9, color:'#ffffff44', fontWeight:700, letterSpacing:'0.1em', textTransform:'uppercase', lineHeight:1 }}>RUNTIME</div>
        </div>
      </div>

      {/* System status pills */}
      <div style={{ display:'flex', gap:6 }}>
        <div style={{ padding:'3px 10px', borderRadius:4, background:'var(--ok-bg)', border:'1px solid var(--ok)44', fontSize:10, fontWeight:700, color:'var(--ok)', display:'flex', gap:5, alignItems:'center' }}>
          <div style={{ width:6, height:6, borderRadius:'50%', background:'var(--ok)', boxShadow:'0 0 5px var(--ok)' }}/>
          SYSTEM OK
        </div>
        <div style={{ padding:'3px 10px', borderRadius:4, background:'rgba(255,255,255,0.05)', border:'1px solid #ffffff22', fontSize:10, color:'#ffffff88', fontFamily:'JetBrains Mono, monospace' }}>
          {taskCount} tasks · {connectedCount} connected
        </div>
      </div>

      <div style={{ flex:1 }}/>

      {/* Time */}
      <div style={{ fontSize:18, fontWeight:700, color:'#fff', fontFamily:'JetBrains Mono, monospace', letterSpacing:'0.05em', paddingRight:12, borderRight:'1px solid #ffffff22' }}>
        {time}
      </div>

      {/* Theme toggle */}
      <button onClick={onToggleTheme} title="Toggle theme" style={{
        background:'rgba(255,255,255,0.08)', border:'1px solid #ffffff22', borderRadius:6, padding:'6px 12px',
        color:'#ffffff88', cursor:'pointer', fontSize:11, fontWeight:600, display:'flex', alignItems:'center', gap:6,
      }}
        onMouseEnter={e=>{ e.currentTarget.style.background='rgba(255,255,255,0.15)'; e.currentTarget.style.color='#fff'; }}
        onMouseLeave={e=>{ e.currentTarget.style.background='rgba(255,255,255,0.08)'; e.currentTarget.style.color='#ffffff88'; }}
      >
        {theme==='dark'
          ? <svg width="14" height="14" viewBox="0 0 24 24" fill="currentColor"><circle cx="12" cy="12" r="5"/><line x1="12" y1="1" x2="12" y2="3" stroke="currentColor" strokeWidth="2" fill="none"/><line x1="12" y1="21" x2="12" y2="23" stroke="currentColor" strokeWidth="2" fill="none"/><line x1="4.22" y1="4.22" x2="5.64" y2="5.64" stroke="currentColor" strokeWidth="2" fill="none"/><line x1="18.36" y1="18.36" x2="19.78" y2="19.78" stroke="currentColor" strokeWidth="2" fill="none"/><line x1="1" y1="12" x2="3" y2="12" stroke="currentColor" strokeWidth="2" fill="none"/><line x1="21" y1="12" x2="23" y2="12" stroke="currentColor" strokeWidth="2" fill="none"/></svg>
          : <svg width="14" height="14" viewBox="0 0 24 24" fill="currentColor"><path d="M21 12.79A9 9 0 1 1 11.21 3 7 7 0 0 0 21 12.79z"/></svg>
        }
        {theme==='dark'?'Light':'Dark'}
      </button>

      {/* Switch to Config */}
      <a href="NCR Vision Config.html" style={{
        background:'rgba(255,255,255,0.08)', border:'1px solid #ffffff22', borderRadius:6, padding:'6px 12px',
        color:'#ffffff88', cursor:'pointer', fontSize:11, fontWeight:600, textDecoration:'none', display:'flex', alignItems:'center', gap:6,
      }}
        onMouseEnter={e=>{ e.currentTarget.style.background='rgba(255,255,255,0.15)'; e.currentTarget.style.color='#fff'; }}
        onMouseLeave={e=>{ e.currentTarget.style.background='rgba(255,255,255,0.08)'; e.currentTarget.style.color='#ffffff88'; }}
      >
        <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round"><circle cx="12" cy="12" r="3"/><path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1-2.83 2.83l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-4 0v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83-2.83l.06-.06A1.65 1.65 0 0 0 4.68 15a1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1 0-4h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 2.83-2.83l.06.06A1.65 1.65 0 0 0 9 4.68a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 4 0v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 2.83l-.06.06A1.65 1.65 0 0 0 19.4 9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 0 4h-.09a1.65 1.65 0 0 0-1.51 1z"/></svg>
        Config
      </a>
    </div>
  );
}

/* ─── Main Runtime App ─── */
function RuntimeApp() {
  const [theme, setTheme] = useState('dark');
  const [alarms, setAlarms] = useState(INIT_ALARMS);
  const [gridTaskIds, setGridTaskIds] = useState([1,2,3]);
  const [floats, setFloats] = useState([]);
  const [fullscreenId, setFullscreenId] = useState(null);
  const [logH, setLogH] = useState(100);
  const [topFloat, setTopFloat] = useState(null);

  useEffect(()=>{ applyTheme(theme); }, [theme]);

  const gridTasks   = TASK_DEFS.filter(t=>gridTaskIds.includes(t.id));
  const floatTasks  = TASK_DEFS.filter(t=>floats.some(f=>f.id===t.id));
  const fsTask      = TASK_DEFS.find(t=>t.id===fullscreenId);
  const allVisible  = [...gridTaskIds, ...floats.map(f=>f.id)];

  const floatTask = useCallback(taskId => {
    setGridTaskIds(prev=>prev.filter(id=>id!==taskId));
    setFloats(prev=>[...prev, { id:taskId, x:200+prev.length*30, y:100+prev.length*30 }]);
  }, []);

  const dockTask = useCallback(taskId => {
    setFloats(prev=>prev.filter(f=>f.id!==taskId));
    setGridTaskIds(prev=>[...prev, taskId]);
  }, []);

  const moveFloat = useCallback((taskId, x, y) => {
    setFloats(prev=>prev.map(f=>f.id===taskId?{...f,x,y}:f));
  }, []);

  const closeFloat = useCallback(taskId => {
    setFloats(prev=>prev.filter(f=>f.id!==taskId));
  }, []);

  const totalDevices    = TASK_DEFS.flatMap(t=>t.devices).length;
  const connectedDevices = TASK_DEFS.flatMap(t=>t.devices).filter(d=>d.ok).length;

  return (
    <div style={{ display:'flex', flexDirection:'column', height:'100vh', background:'var(--bg)', fontFamily:'Space Grotesk, sans-serif', color:'var(--text)', overflow:'hidden' }}>
      <RuntimeTopBar theme={theme} onToggleTheme={()=>setTheme(t=>t==='dark'?'light':'dark')} taskCount={TASK_DEFS.length} connectedCount={connectedDevices}/>
      <AlarmBar alarms={alarms} onAck={id=>setAlarms(prev=>prev.filter(a=>a.id!==id))} onAckAll={()=>setAlarms([])}/>

      {/* Main content */}
      <div style={{ flex:1, display:'flex', flexDirection:'column', overflow:'hidden', position:'relative' }}>
        {fullscreenId && fsTask ? (
          <div style={{ flex:1, padding:8, overflow:'hidden' }}>
            <div style={{ position:'absolute', top:12, right:12, zIndex:10 }}>
              <button onClick={()=>setFullscreenId(null)} style={{ background:'var(--surface)', border:'1px solid var(--border)', borderRadius:5, padding:'6px 14px', fontSize:11, fontWeight:700, color:'var(--text2)', cursor:'pointer' }}>
                ← Back to Grid
              </button>
            </div>
            <TaskMonitorCard task={fsTask} compact={false}/>
          </div>
        ) : (
          <TaskGrid tasks={gridTasks} onFloat={floatTask} onFullscreen={id=>setFullscreenId(id)}/>
        )}

        {/* Floating windows */}
        {floatTasks.map(task => {
          const f = floats.find(f=>f.id===task.id);
          return (
            <FloatingTaskCard key={task.id} task={task} x={f.x} y={f.y}
              onMove={moveFloat} onClose={closeFloat} onDock={dockTask}
              zIndex={topFloat===task.id?300:290} onFocus={setTopFloat}
            />
          );
        })}
      </div>

      <EventLogStrip height={logH} onResize={setLogH}/>
    </div>
  );
}

const root = ReactDOM.createRoot(document.getElementById('root'));
root.render(<RuntimeApp/>);
