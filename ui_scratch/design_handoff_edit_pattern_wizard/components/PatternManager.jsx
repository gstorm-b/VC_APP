
// PatternManager.jsx  v2
// Layout:
//   Pane 1 (flex): Monitor feed + Toolbar  (top)  /  Result Table  (bottom, resizable)
//   Pane 2 (300px): Pattern Library tree  (top)   /  Pattern Setting (thumb+props, bottom)
//   Pane 3 (260px): PropertyPanel — group/pattern config via property inspector
const { useState, useRef, useEffect, useCallback } = React;

/* ─── seed data ─────────────────────────────────────── */
let _pmid = 0;
const pmid = () => `pm${++_pmid}`;

const mkEdge = () => ({
  threshLower:100, threshUpper:200, kernelSize:3,
  blurW:5, blurH:5, greediness:0.0,
  minReduceLength:32, tSamples:3,
  invertBinary:true, subPixel:false, stopAtLayer1:false,
});

const mkPat = (name, number, learned=false, overrides={}) => ({
  id:pmid(), name, number, learned,
  minScore:0.80, angle:0, tolAngle:180, maxOverlap:0.1,
  pickX:0, pickY:0, matchType:'EdgeBased', edge:mkEdge(),
  ...overrides,
});

const mkGrp = (name, number, patterns=[]) => ({
  id:pmid(), name, number,
  lowWorkpieceRatio:1.5,
  pickBoxW:80, pickBoxH:60, pickBoxDist:0, pickBoxAngle:0,
  patterns,
});

const INIT_GROUPS = [
  mkGrp('Part_A_Front', 1, [
    mkPat('Face_Front',  1, true,  { minScore:0.90, tolAngle:180 }),
    mkPat('Face_Side',   2, true,  { minScore:0.88, tolAngle:90  }),
    mkPat('Face_Corner', 3, false, { minScore:0.85, tolAngle:180 }),
  ]),
  mkGrp('Part_B_Side', 2, [
    mkPat('Side_01', 1, true,  { minScore:0.92, tolAngle:90  }),
    mkPat('Side_02', 2, false, { minScore:0.85, tolAngle:180 }),
  ]),
  mkGrp('Part_C_Empty', 3, []),
];

/* ─── design tokens ──────────────────────────────────── */
const C = {
  bg:'#1e1e1e', surf:'#252526', surf2:'#2d2d2d',
  bd:'#3c3c3c', bd2:'#454545',
  txt:'#cccccc', txt2:'#9a9a9a', txt3:'#454545',
  acc:'#2b8ce8', ok:'#22d17a', warn:'#f5a623', err:'#e84040',
  hd:'#1e1e1e',
};
const mono = 'JetBrains Mono, monospace';
const sans = 'Space Grotesk, sans-serif';
const fi = {
  background:C.bg, border:`1px solid ${C.bd}`, borderRadius:4,
  padding:'4px 7px', color:C.txt, fontSize:11, fontFamily:mono,
  outline:'none', width:'100%', boxSizing:'border-box',
};

/* ─── tiny atoms ─────────────────────────────────────── */
function NumBadge({ n, color=C.acc, size=18, title:tt }) {
  return (
    <div title={tt} style={{ width:size,height:size,borderRadius:3,flexShrink:0,
      background:`${color}22`,border:`1px solid ${color}66`,
      display:'flex',alignItems:'center',justifyContent:'center' }}>
      <span style={{ fontFamily:mono,fontSize:size*0.5,fontWeight:800,color,lineHeight:1 }}>{n}</span>
    </div>
  );
}

function Dot({ ok }) {
  const c=ok?C.ok:C.err;
  return <div style={{ width:6,height:6,borderRadius:'50%',background:c,boxShadow:ok?`0 0 4px ${c}`:'none',flexShrink:0 }}/>;
}

function TBtn({ children, onClick, title:tt, active, disabled, color }) {
  const [h,setH]=useState(false);
  return (
    <button onClick={onClick} title={tt} disabled={disabled} style={{
      background:active?`${C.acc}22`:h?C.surf2:'transparent',
      border:`1px solid ${active?C.acc:h?C.bd2:'transparent'}`,
      borderRadius:4, padding:'4px 10px',
      color:disabled?C.txt3:active?C.acc:h?C.txt:(color||C.txt2),
      fontSize:11,fontWeight:600,cursor:disabled?'default':'pointer',
      display:'flex',alignItems:'center',gap:5,fontFamily:sans,
      transition:'all 0.12s',flexShrink:0,whiteSpace:'nowrap',
    }} onMouseEnter={()=>setH(true)} onMouseLeave={()=>setH(false)}>{children}</button>
  );
}

function IBtn({ icon, title:tt, onClick, danger }) {
  const [h,setH]=useState(false);
  return (
    <button onClick={onClick} title={tt} style={{
      background:h?(danger?'#2a1a1a':C.surf2):'transparent',
      border:'none',cursor:'pointer',padding:'2px 4px',borderRadius:3,
      color:h?(danger?C.err:C.txt2):C.txt3,display:'flex',alignItems:'center',
    }} onMouseEnter={()=>setH(true)} onMouseLeave={()=>setH(false)}>{icon}</button>
  );
}

/* ─── inline rename ─────────────────────────────────── */
function InlineEdit({ value, onCommit }) {
  const [ed,setEd]=useState(false);
  const [v,setV]=useState(value);
  const ref=useRef(null);
  useEffect(()=>{ if(ed) ref.current?.select(); },[ed]);
  const commit=()=>{ setEd(false); if(v.trim()&&v.trim()!==value) onCommit(v.trim()); else setV(value); };
  if(!ed) return <span onDoubleClick={()=>setEd(true)} title="Double-click to rename" style={{ cursor:'text',fontFamily:mono,fontSize:11 }}>{value}</span>;
  return <input ref={ref} value={v} onChange={e=>setV(e.target.value)} autoFocus
    onBlur={commit} onKeyDown={e=>{ if(e.key==='Enter')commit(); if(e.key==='Escape'){setEd(false);setV(value);} }}
    style={{ ...fi,padding:'1px 4px',fontSize:11 }}/>;
}

/* ─── dialogs ────────────────────────────────────────── */
function Modal({ title, subtitle, onClose, width=420, children }) {
  return (
    <div onClick={onClose} style={{ position:'fixed',inset:0,background:'rgba(5,9,18,0.78)',display:'flex',alignItems:'center',justifyContent:'center',zIndex:2000,backdropFilter:'blur(3px)' }}>
      <div onClick={e=>e.stopPropagation()} style={{ width,background:'#2d2d2d',border:`1px solid ${C.bd2}`,borderRadius:9,overflow:'hidden',boxShadow:'0 24px 64px rgba(0,0,0,0.6)' }}>
        <div style={{ padding:'12px 18px',borderBottom:`1px solid ${C.bd}`,background:C.hd,display:'flex',alignItems:'flex-start',justifyContent:'space-between' }}>
          <div>
            <div style={{ fontSize:13,fontWeight:700,color:C.txt }}>{title}</div>
            {subtitle&&<div style={{ fontSize:10,color:C.txt3,marginTop:2 }}>{subtitle}</div>}
          </div>
          <IBtn icon={<svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.2" strokeLinecap="round"><line x1="18" y1="6" x2="6" y2="18"/><line x1="6" y1="6" x2="18" y2="18"/></svg>} onClick={onClose}/>
        </div>
        <div style={{ padding:'20px' }}>{children}</div>
      </div>
    </div>
  );
}

const dlgFi = { ...fi, padding:'7px 10px', fontSize:12, border:`1px solid ${C.bd2}` };
function DlgField({ label, hint, error, children }) {
  return (
    <div style={{ marginBottom:14 }}>
      <label style={{ fontSize:10,fontWeight:700,color:C.txt3,textTransform:'uppercase',letterSpacing:'0.08em',display:'block',marginBottom:5 }}>{label}</label>
      {children}
      {error&&<div style={{ fontSize:10,color:C.err,marginTop:3 }}>{error}</div>}
      {hint&&!error&&<div style={{ fontSize:10,color:C.txt3,marginTop:3 }}>{hint}</div>}
    </div>
  );
}

function AddGroupDlg({ usedNums, usedNames, onConfirm, onClose }) {
  const [name,setName]=useState(''); const [num,setNum]=useState('');
  const n=parseInt(num)||0;
  const ne=usedNames.includes(name.trim())&&name.trim()?'Name already exists':null;
  const nue=num&&(usedNums.includes(n)||n<1)?(n<1?'Must be ≥ 1':'Already exists'):null;
  const ok=name.trim()&&num&&!ne&&!nue;
  return (
    <Modal title="New Pattern Group" onClose={onClose}>
      <DlgField label="Group Name" error={ne}>
        <input style={{ ...dlgFi,borderColor:ne?C.err:C.bd2 }} value={name} onChange={e=>setName(e.target.value)} placeholder="e.g. Part_A" autoFocus
          onFocus={e=>e.target.style.borderColor=C.acc} onBlur={e=>e.target.style.borderColor=ne?C.err:C.bd2}/>
      </DlgField>
      <DlgField label="Group Number" error={nue} hint="Runtime selector — send this number via PLC to activate this group.">
        <input style={{ ...dlgFi,borderColor:nue?C.err:C.bd2 }} value={num} onChange={e=>setNum(e.target.value)} type="number" min={1} placeholder="1"
          onFocus={e=>e.target.style.borderColor=C.acc} onBlur={e=>e.target.style.borderColor=nue?C.err:C.bd2}/>
      </DlgField>
      <div style={{ display:'flex',gap:8,justifyContent:'flex-end' }}>
        <TBtn onClick={onClose}>Cancel</TBtn>
        <TBtn onClick={()=>ok&&onConfirm(name.trim(),n)} active={!!ok} disabled={!ok}>Create Group</TBtn>
      </div>
    </Modal>
  );
}

function AddPatternDlg({ groupName, usedNums, usedNames, onConfirm, onClose }) {
  const [name,setName]=useState(''); const [num,setNum]=useState('');
  const n=parseInt(num)||0;
  const ne=usedNames.includes(name.trim())&&name.trim()?'Name already exists in group':null;
  const nue=num&&(usedNums.includes(n)||n<1)?(n<1?'Must be ≥ 1':'Already exists in group'):null;
  const ok=name.trim()&&num&&!ne&&!nue;
  return (
    <Modal title="Add Pattern" subtitle={`Group: ${groupName}`} onClose={onClose} width={440}>
      <DlgField label="Pattern Name" error={ne}>
        <input style={{ ...dlgFi,borderColor:ne?C.err:C.bd2 }} value={name} onChange={e=>setName(e.target.value)} placeholder="e.g. Front_face" autoFocus
          onFocus={e=>e.target.style.borderColor=C.acc} onBlur={e=>e.target.style.borderColor=ne?C.err:C.bd2}/>
      </DlgField>
      <DlgField label="Pattern Number" error={nue} hint="Pattern ID written to output register on match.">
        <input style={{ ...dlgFi,borderColor:nue?C.err:C.bd2 }} value={num} onChange={e=>setNum(e.target.value)} type="number" min={1} placeholder="1"
          onFocus={e=>e.target.style.borderColor=C.acc} onBlur={e=>e.target.style.borderColor=nue?C.err:C.bd2}/>
      </DlgField>
      <div style={{ background:C.hd,border:`1px solid ${C.bd}`,borderRadius:5,padding:'8px 12px',marginBottom:14,display:'flex',alignItems:'center',gap:8 }}>
        <IcoCamera size={13} color={C.txt3}/>
        <span style={{ fontSize:10,color:C.txt3 }}>Capture or load image after creation.</span>
      </div>
      <div style={{ display:'flex',gap:8,justifyContent:'flex-end' }}>
        <TBtn onClick={onClose}>Cancel</TBtn>
        <TBtn onClick={()=>ok&&onConfirm(name.trim(),n)} active={!!ok} disabled={!ok}>Add Pattern</TBtn>
      </div>
    </Modal>
  );
}

/* ─── PANE 1A: Monitor ──────────────────────────────── */
const MOCK_RESULTS = [
  { idx:1, patNum:1, patName:'Face_Front',  score:0.967, x:823.4, y:512.1, angle:12.4, ok:true  },
  { idx:2, patNum:2, patName:'Face_Side',   score:0.881, x:614.2, y:480.9, angle:45.2, ok:true  },
  { idx:3, patNum:1, patName:'Face_Front',  score:0.812, x:420.7, y:310.5, angle:178.1,ok:true  },
  { idx:4, patNum:3, patName:'Face_Corner', score:0.541, x:290.0, y:180.3, angle:92.0, ok:false },
];

function MonitorPane({ groups, activeGroupId, onSetActive, resultTableH, onResizeResultTable }) {
  const [view,setView]   = useState('raw');
  const [showROI,setShowROI] = useState(false);
  const [useROI,setUseROI]   = useState(false);
  const [running,setRunning] = useState(false);
  const [results,setResults] = useState([]);
  const [kpi,setKpi]         = useState({ found:0, time:0, below:0, score:0, x:0,y:0,z:0,r:0 });
  const [selResult,setSelResult] = useState(null);

  const activeGroup = groups.find(g=>g.id===activeGroupId);

  const runMatch = () => {
    setRunning(true);
    setTimeout(()=>{
      setResults(MOCK_RESULTS);
      setKpi({ found:4, time:124, below:1, score:0.967, x:823.4, y:512.1, z:380.0, r:12.4 });
      setRunning(false); setView('result');
    }, 800);
  };

  const dragRef = useRef(null);
  const handleDragStart = useCallback(e=>{
    dragRef.current = { startY:e.clientY, startH:resultTableH };
    const onMove = e => onResizeResultTable(Math.max(80, Math.min(360, dragRef.current.startH + dragRef.current.startY - e.clientY)));
    const onUp   = () => { window.removeEventListener('mousemove',onMove); window.removeEventListener('mouseup',onUp); };
    window.addEventListener('mousemove',onMove);
    window.addEventListener('mouseup',onUp);
  },[resultTableH, onResizeResultTable]);

  return (
    <div style={{ display:'flex',flexDirection:'column',height:'100%',overflow:'hidden' }}>
      {/* Toolbar */}
      <div style={{ height:44,background:C.hd,borderBottom:`1px solid ${C.bd}`,display:'flex',alignItems:'center',padding:'0 10px',gap:5,flexShrink:0 }}>
        <TBtn title="Capture from camera">
          <svg width="11" height="11" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round"><polygon points="23 7 16 12 23 17 23 7"/><rect x="1" y="5" width="15" height="14" rx="2"/></svg>Trigger
        </TBtn>
        <TBtn title="Load image from disk">
          <svg width="11" height="11" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round"><path d="M22 19a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5l2 3h9a2 2 0 0 1 2 2z"/></svg>Open Image
        </TBtn>
        <TBtn title="Define matching workspace ROI">
          <svg width="11" height="11" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round"><rect x="3" y="3" width="18" height="18" rx="2"/><line x1="9" y1="3" x2="9" y2="21" opacity="0.4"/><line x1="15" y1="3" x2="15" y2="21" opacity="0.4"/><line x1="3" y1="9" x2="21" y2="9" opacity="0.4"/><line x1="3" y1="15" x2="21" y2="15" opacity="0.4"/></svg>Workspace
        </TBtn>
        <div style={{ width:1,height:20,background:C.bd }}/>
        <span style={{ fontSize:10,color:C.txt3,flexShrink:0 }}>Active:</span>
        <select value={activeGroupId||''} onChange={e=>onSetActive(e.target.value)}
          style={{ ...fi,width:130,padding:'3px 7px',fontSize:11 }}>
          {groups.map(g=><option key={g.id} value={g.id}>#{g.number} {g.name}</option>)}
        </select>
        <div style={{ width:1,height:20,background:C.bd }}/>
        <label style={{ display:'flex',alignItems:'center',gap:4,cursor:'pointer',fontSize:11,color:C.txt2,flexShrink:0 }}>
          <input type="checkbox" checked={showROI} onChange={e=>setShowROI(e.target.checked)} style={{ accentColor:C.acc }}/>Show ROI
        </label>
        <label style={{ display:'flex',alignItems:'center',gap:4,cursor:'pointer',fontSize:11,color:C.txt2,flexShrink:0 }}>
          <input type="checkbox" checked={useROI} onChange={e=>setUseROI(e.target.checked)} style={{ accentColor:C.acc }}/>Use ROI
        </label>
        <div style={{ flex:1 }}/>
        <div style={{ padding:'3px 10px',borderRadius:12,fontSize:9,fontWeight:700,
          background:running?`${C.acc}22`:`${C.txt3}22`,
          border:`1px solid ${running?C.acc:C.bd}`,
          color:running?C.acc:C.txt3 }}>
          {running?'● MATCHING…':'● IDLE'}
        </div>
        <TBtn onClick={runMatch} active disabled={running} title="Run matching on current image">
          <svg width="10" height="10" viewBox="0 0 24 24" fill="currentColor"><polygon points="5 3 19 12 5 21"/></svg>
          Run Matching
        </TBtn>
      </div>

      {/* View tab strip */}
      <div style={{ height:30,background:'#252526',borderBottom:`1px solid ${C.bd}`,display:'flex',alignItems:'stretch',flexShrink:0 }}>
        <span style={{ fontSize:9,fontWeight:700,color:C.txt3,textTransform:'uppercase',letterSpacing:'0.1em',display:'flex',alignItems:'center',padding:'0 10px',borderRight:`1px solid ${C.bd}` }}>MONITOR</span>
        {['raw','result'].map(v=>(
          <button key={v} onClick={()=>setView(v)} style={{ background:'transparent',border:'none',borderBottom:`2px solid ${view===v?C.acc:'transparent'}`,padding:'0 14px',cursor:'pointer',fontSize:11,fontWeight:view===v?700:400,color:view===v?C.txt:C.txt2,fontFamily:sans }}>
            {v==='raw'?'Raw':'Result'}
          </button>
        ))}
      </div>

      {/* Camera feed */}
      <div style={{ flex:1,background:'#181818',position:'relative',overflow:'hidden',minHeight:0 }}>
        <svg width="100%" height="100%" style={{ position:'absolute',inset:0,opacity:0.2 }}>
          {Array.from({length:16},(_,i)=><line key={`h${i}`} x1="0" y1={`${i*6.25}%`} x2="100%" y2={`${i*6.25}%`} stroke="#3c3c3c" strokeWidth="0.5"/>)}
          {Array.from({length:20},(_,i)=><line key={`v${i}`} x1={`${i*5}%`} y1="0" x2={`${i*5}%`} y2="100%" stroke="#3c3c3c" strokeWidth="0.5"/>)}
        </svg>
        {/* Brackets */}
        {[['top','left'],['top','right'],['bottom','left'],['bottom','right']].map(([v,h],i)=>(
          <div key={i} style={{ position:'absolute',[v]:12,[h]:12,width:18,height:18,
            [`border${v.charAt(0).toUpperCase()+v.slice(1)}`]:`1.5px solid ${C.acc}`,
            [`border${h.charAt(0).toUpperCase()+h.slice(1)}`]:`1.5px solid ${C.acc}` }}/>
        ))}
        {/* Reticle */}
        <div style={{ position:'absolute',inset:0,display:'flex',alignItems:'center',justifyContent:'center' }}>
          <svg width="80" height="60">
            <rect x="4" y="4" width="72" height="52" rx="2" fill="none" stroke={C.acc} strokeWidth="1" strokeDasharray="8 4" opacity="0.4"/>
            <line x1="40" y1="0" x2="40" y2="60" stroke={`${C.acc}33`} strokeWidth="0.8"/>
            <line x1="0" y1="30" x2="80" y2="30" stroke={`${C.acc}33`} strokeWidth="0.8"/>
            <circle cx="40" cy="30" r="5" fill="none" stroke={C.acc} strokeWidth="1.5" opacity="0.5"/>
            <circle cx="40" cy="30" r="1.5" fill={C.acc} opacity="0.7"/>
          </svg>
        </div>
        {/* ROI */}
        {(showROI||useROI)&&<div style={{ position:'absolute',top:'18%',left:'14%',right:'14%',bottom:'18%',border:`1.5px dashed ${C.warn}`,borderRadius:2,pointerEvents:'none' }}>
          <span style={{ position:'absolute',top:-14,left:0,fontSize:8,color:C.warn,fontFamily:mono,fontWeight:700 }}>WORKSPACE ROI</span>
        </div>}
        {/* Result bboxes */}
        {view==='result'&&results.map((r,i)=>(
          <div key={r.idx} onClick={()=>setSelResult(selResult?.idx===r.idx?null:r)}
            style={{ position:'absolute',
              top:`${20+i*10}%`, left:`${15+i*14}%`,
              width:72, height:52,
              border:`1.5px solid ${r.ok?(selResult?.idx===r.idx?'#fff':C.ok):C.err}`,
              borderRadius:2, cursor:'pointer',
              boxShadow:selResult?.idx===r.idx?`0 0 0 1px ${C.acc}`:'none',
            }}>
            <div style={{ position:'absolute',top:-14,left:0,background:r.ok?`${C.ok}22`:`${C.err}22`,border:`1px solid ${r.ok?C.ok:C.err}44`,borderRadius:3,padding:'1px 5px',whiteSpace:'nowrap' }}>
              <span style={{ fontSize:8,fontFamily:mono,color:r.ok?C.ok:C.err,fontWeight:700 }}>#{r.patNum} {r.score.toFixed(3)}</span>
            </div>
          </div>
        ))}
        {/* Overlay labels */}
        <div style={{ position:'absolute',bottom:8,left:10,fontSize:9,color:C.txt3,fontFamily:mono }}>Camera Feed Placeholder · 2448×2048</div>
        {activeGroup&&<div style={{ position:'absolute',top:10,right:10,fontSize:9,fontFamily:mono,fontWeight:700,color:C.warn }}>Active: #{activeGroup.number}</div>}
        {running&&<div style={{ position:'absolute',inset:0,background:'rgba(3,8,16,0.65)',display:'flex',alignItems:'center',justifyContent:'center' }}>
          <span style={{ fontSize:14,color:C.acc,fontFamily:mono,fontWeight:700 }}>Matching…</span>
        </div>}
      </div>

      {/* KPI strip */}
      <div style={{ background:C.surf,borderTop:`1px solid ${C.bd}`,display:'flex',flexShrink:0 }}>
        {[['FOUND',kpi.found,C.acc,''],['EXEC TIME',kpi.time,C.txt,'ms'],['BELOW THRESH',kpi.below,C.warn,''],['BEST SCORE',kpi.score.toFixed(3),C.ok,'']].map(([lbl,val,color,unit])=>(
          <div key={lbl} style={{ flex:1,padding:'6px 10px',borderRight:`1px solid ${C.bd}`,display:'flex',flexDirection:'column',gap:1 }}>
            <span style={{ fontSize:8,fontWeight:700,color:C.txt3,textTransform:'uppercase',letterSpacing:'0.1em' }}>{lbl}</span>
            <span style={{ fontSize:16,fontWeight:800,color:kpi.found>0?color:C.txt3,fontFamily:mono,lineHeight:1.1 }}>{kpi.found>0?val:'—'}{kpi.found>0&&unit?' '+unit:''}</span>
          </div>
        ))}
        <div style={{ flex:2,padding:'6px 10px',display:'flex',flexDirection:'column',gap:2 }}>
          <span style={{ fontSize:8,fontWeight:700,color:C.txt3,textTransform:'uppercase',letterSpacing:'0.1em' }}>PICK POSITION</span>
          <div style={{ display:'flex',gap:12 }}>
            {[['X',kpi.x],['Y',kpi.y],['Z',kpi.z],['R',kpi.r]].map(([ax,val])=>(
              <div key={ax} style={{ display:'flex',gap:4,alignItems:'baseline' }}>
                <span style={{ fontSize:9,color:C.acc,fontFamily:mono,fontWeight:700 }}>{ax}</span>
                <span style={{ fontSize:11,color:kpi.found>0?C.txt:C.txt3,fontFamily:mono }}>{kpi.found>0?val:'0.00'}</span>
              </div>
            ))}
          </div>
        </div>
      </div>

      {/* ── Result Table (resizable) ── */}
      <div style={{ height:resultTableH,background:C.hd,borderTop:`1px solid ${C.bd}`,display:'flex',flexDirection:'column',flexShrink:0 }}>
        {/* drag handle */}
        <div onMouseDown={handleDragStart} style={{ height:4,cursor:'ns-resize',flexShrink:0 }}
          onMouseEnter={e=>e.currentTarget.style.background=`${C.acc}55`}
          onMouseLeave={e=>e.currentTarget.style.background='transparent'}/>
        {/* table header */}
        <div style={{ display:'flex',alignItems:'center',padding:'3px 12px',borderBottom:`1px solid ${C.bd}`,flexShrink:0 }}>
          <span style={{ fontSize:9,fontWeight:700,color:C.txt3,textTransform:'uppercase',letterSpacing:'0.1em',flex:1 }}>Match Results</span>
          <span style={{ fontSize:9,color:C.txt3,fontFamily:mono }}>{results.length} objects found</span>
        </div>
        {/* column headers */}
        <div style={{ display:'grid',gridTemplateColumns:'32px 44px 1fr 72px 80px 80px 64px 44px',
          padding:'3px 12px',borderBottom:`1px solid ${C.bd}`,background:C.surf,flexShrink:0 }}>
          {['#','Pat #','Pattern Name','Score','X','Y','Angle','OK'].map(h=>(
            <span key={h} style={{ fontSize:9,fontWeight:700,color:C.txt3,textTransform:'uppercase',letterSpacing:'0.06em' }}>{h}</span>
          ))}
        </div>
        {/* rows */}
        <div style={{ flex:1,overflowY:'auto' }}>
          {results.length===0?(
            <div style={{ padding:'12px',color:C.txt3,fontSize:11,textAlign:'center' }}>No results — run matching first</div>
          ):results.map(r=>{
            const isSel = selResult?.idx===r.idx;
            return (
              <div key={r.idx} onClick={()=>setSelResult(isSel?null:r)}
                style={{ display:'grid',gridTemplateColumns:'32px 44px 1fr 72px 80px 80px 64px 44px',
                  padding:'4px 12px',cursor:'pointer',userSelect:'none',
                  background:isSel?`${C.acc}18`:'transparent',
                  borderLeft:`2px solid ${isSel?C.acc:'transparent'}`,
                  borderBottom:`1px solid ${C.bd}`,
                }}
                onMouseEnter={e=>{ if(!isSel) e.currentTarget.style.background=C.surf2; }}
                onMouseLeave={e=>{ if(!isSel) e.currentTarget.style.background='transparent'; }}
              >
                <span style={{ fontSize:11,fontFamily:mono,color:C.txt2 }}>{r.idx}</span>
                <span style={{ fontSize:11,fontFamily:mono }}><NumBadge n={r.patNum} color="#9cdcfe" size={16} title="Pattern number (output ID)"/></span>
                <span style={{ fontSize:11,fontFamily:mono,color:C.txt,overflow:'hidden',textOverflow:'ellipsis',whiteSpace:'nowrap' }}>{r.patName}</span>
                <span style={{ fontSize:11,fontFamily:mono,color:r.score>=0.85?C.ok:r.score>=0.70?C.warn:C.err,fontWeight:700 }}>{r.score.toFixed(3)}</span>
                <span style={{ fontSize:11,fontFamily:mono,color:C.txt2 }}>{r.x.toFixed(1)}</span>
                <span style={{ fontSize:11,fontFamily:mono,color:C.txt2 }}>{r.y.toFixed(1)}</span>
                <span style={{ fontSize:11,fontFamily:mono,color:C.txt2 }}>{r.angle.toFixed(1)}°</span>
                <span style={{ fontSize:11,fontWeight:800,color:r.ok?C.ok:C.err,fontFamily:mono }}>{r.ok?'OK':'NG'}</span>
              </div>
            );
          })}
        </div>
      </div>
    </div>
  );
}

/* ─── PANE 2: Pattern Library + Pattern Setting ─────── */
function PatternThumb({ learned }) {
  return (
    <div style={{ width:'100%',aspectRatio:'4/3',background:'#181818',borderRadius:4,border:`1px solid ${C.bd}`,
      position:'relative',overflow:'hidden',display:'flex',alignItems:'center',justifyContent:'center' }}>
      <svg width="100%" height="100%" style={{ position:'absolute',inset:0,opacity:0.2 }}>
        {[1,2,3,4].map(i=><line key={`h${i}`} x1="0" y1={`${i*20}%`} x2="100%" y2={`${i*20}%`} stroke={C.bd} strokeWidth="0.5"/>)}
        {[1,2,3,4].map(i=><line key={`v${i}`} x1={`${i*20}%`} y1="0" x2={`${i*20}%`} y2="100%" stroke={C.bd} strokeWidth="0.5"/>)}
      </svg>
      {learned?(
        <>
          <svg width="64" height="48" style={{ position:'relative',zIndex:1 }}>
            <rect x="4" y="4" width="56" height="40" rx="2" fill="none" stroke={C.acc} strokeWidth="1.2" strokeDasharray="6 3"/>
            <line x1="32" y1="0" x2="32" y2="48" stroke={`${C.acc}44`} strokeWidth="0.8"/>
            <line x1="0" y1="24" x2="64" y2="24" stroke={`${C.acc}44`} strokeWidth="0.8"/>
            <circle cx="32" cy="24" r="4" fill="none" stroke={C.acc} strokeWidth="1.5"/>
            <circle cx="32" cy="24" r="1.5" fill={C.acc}/>
          </svg>
          <div style={{ position:'absolute',bottom:4,left:0,right:0,textAlign:'center' }}>
            <span style={{ fontSize:8,fontFamily:mono,color:C.ok,fontWeight:700 }}>● LEARNED</span>
          </div>
        </>
      ):(
        <span style={{ fontSize:10,color:C.txt3,textAlign:'center',fontFamily:mono,zIndex:1,position:'relative' }}>Not learned</span>
      )}
    </div>
  );
}

function PropRow({ label, children }) {
  return (
    <div style={{ display:'grid',gridTemplateColumns:'1fr 1fr',alignItems:'center',
      padding:'3px 10px 3px 12px',borderBottom:`1px solid #1e1e1e`,minHeight:26 }}>
      <span style={{ fontSize:11,color:'#7a7a7a',fontWeight:500,overflow:'hidden',textOverflow:'ellipsis',whiteSpace:'nowrap' }} title={label}>{label}</span>
      {children}
    </div>
  );
}

function FInput({ value, onChange, type='text', unit, step=1, min, max }) {
  const [foc,setFoc]=useState(false);
  return (
    <div style={{ display:'flex',alignItems:'center',gap:4 }}>
      <input type={type} value={value} step={step} min={min} max={max}
        onChange={e=>onChange(type==='number'?parseFloat(e.target.value)||0:e.target.value)}
        onFocus={()=>setFoc(true)} onBlur={()=>setFoc(false)}
        style={{ ...fi,padding:'2px 6px',fontSize:11,flex:1,borderColor:foc?C.acc:C.bd }}/>
      {unit&&<span style={{ fontSize:9,color:C.txt3,flexShrink:0,fontFamily:mono }}>{unit}</span>}
    </div>
  );
}

function FToggle({ value, onChange }) {
  return (
    <div onClick={()=>onChange(!value)} style={{ width:28,height:15,borderRadius:8,cursor:'pointer',
      background:value?C.acc:'#3c3c3c',position:'relative',transition:'background 0.18s',
      border:`1px solid ${value?C.acc:C.bd2}` }}>
      <div style={{ position:'absolute',top:2,left:value?14:2,width:9,height:9,borderRadius:'50%',background:'#fff',transition:'left 0.18s' }}/>
    </div>
  );
}

function PatternSetting({ sel, groups, onUpdate, onLearn }) {
  const group   = sel ? groups.find(g=>g.id===sel.groupId) : null;
  const pattern = group && sel?.patternId ? group.patterns.find(p=>p.id===sel.patternId) : null;

  /* nothing selected */
  if (!group) return (
    <div style={{ display:'flex',alignItems:'center',justifyContent:'center',color:C.txt3,fontSize:11,padding:16,textAlign:'center',minHeight:80 }}>
      Select a group or pattern
    </div>
  );

  const updP = (key,val) => onUpdate(group.id, pattern?.id, key, val);
  const updE = (key,val) => onUpdate(group.id, pattern?.id, `edge.${key}`, val);
  const updG = (key,val) => onUpdate(group.id, null, key, val);

  /* ── Group selected, no pattern ── */
  if (!pattern) return (
    <div>
      <div style={{ padding:'4px 10px',background:C.hd,borderBottom:`1px solid #252526` }}>
        <span style={{ fontSize:9,fontWeight:700,color:C.txt3,textTransform:'uppercase',letterSpacing:'0.1em' }}>Group Config</span>
      </div>
      <PropRow label="Group Name">
        <span style={{ fontSize:11,fontFamily:mono,color:C.txt2,padding:'2px 6px' }}>{group.name}</span>
      </PropRow>
      <PropRow label="Group Number">
        <div style={{ display:'flex',alignItems:'center',gap:6 }}>
          <span style={{ fontSize:11,fontFamily:mono,color:C.warn,fontWeight:700,padding:'2px 6px' }}>{group.number}</span>
          <span style={{ fontSize:9,color:C.txt3 }}>runtime selector</span>
        </div>
      </PropRow>
      <div style={{ padding:'4px 10px',background:C.hd,borderBottom:`1px solid #252526`,borderTop:`1px solid ${C.bd}`,marginTop:2 }}>
        <span style={{ fontSize:9,fontWeight:700,color:C.txt3,textTransform:'uppercase',letterSpacing:'0.1em' }}>Picking Box</span>
      </div>
      <PropRow label="Low Workpiece Ratio"><FInput value={group.lowWorkpieceRatio} onChange={v=>updG('lowWorkpieceRatio',v)} type="number" step={0.1}/></PropRow>
      <PropRow label="Box Width"><FInput value={group.pickBoxW} onChange={v=>updG('pickBoxW',v)} type="number" unit="px"/></PropRow>
      <PropRow label="Box Height"><FInput value={group.pickBoxH} onChange={v=>updG('pickBoxH',v)} type="number" unit="px"/></PropRow>
      <PropRow label="Distance"><FInput value={group.pickBoxDist} onChange={v=>updG('pickBoxDist',v)} type="number" unit="px"/></PropRow>
      <PropRow label="Angle"><FInput value={group.pickBoxAngle} onChange={v=>updG('pickBoxAngle',v)} type="number" unit="°"/></PropRow>
    </div>
  );

  /* ── Pattern selected ── */
  return (
    <div>
      {/* Thumbnail + learn */}
      <div style={{ padding:'8px 10px',borderBottom:`1px solid ${C.bd}` }}>
        <div style={{ display:'flex',alignItems:'center',justifyContent:'space-between',marginBottom:6 }}>
          <span style={{ fontSize:9,fontWeight:700,color:C.txt3,textTransform:'uppercase',letterSpacing:'0.1em' }}>
            #{pattern.number} {pattern.name}
          </span>
          <span style={{ fontSize:9,fontWeight:800,color:pattern.learned?C.ok:C.err }}>{pattern.learned?'LEARNED':'NOT LEARNED'}</span>
        </div>
        <PatternThumb learned={pattern.learned}/>
        <div style={{ display:'flex',gap:5,marginTop:7 }}>
          <TBtn onClick={()=>onLearn(group.id,pattern.id)} color={C.ok}>
            <svg width="11" height="11" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round"><polygon points="23 7 16 12 23 17 23 7"/><rect x="1" y="5" width="15" height="14" rx="2"/></svg>
            Trigger & Learn
          </TBtn>
          <TBtn>
            <svg width="11" height="11" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round"><path d="M22 19a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5l2 3h9a2 2 0 0 1 2 2z"/></svg>
            Open Image
          </TBtn>
        </div>
      </div>

      {/* Match settings */}
      <div style={{ padding:'4px 10px',background:C.hd,borderBottom:`1px solid #252526` }}>
        <span style={{ fontSize:9,fontWeight:700,color:C.txt3,textTransform:'uppercase',letterSpacing:'0.1em' }}>Match Settings</span>
      </div>
      <PropRow label="Pattern Number">
        <div style={{ display:'flex',alignItems:'center',gap:6 }}>
          <span style={{ fontSize:11,fontFamily:mono,color:'#9cdcfe',fontWeight:700,padding:'2px 6px' }}>{pattern.number}</span>
          <span style={{ fontSize:9,color:C.txt3 }}>output ID</span>
        </div>
      </PropRow>
      <PropRow label="Min Score"><FInput value={pattern.minScore} onChange={v=>updP('minScore',v)} type="number" step={0.01} min={0} max={1}/></PropRow>
      <PropRow label="Angle"><FInput value={pattern.angle} onChange={v=>updP('angle',v)} type="number" unit="°"/></PropRow>
      <PropRow label="Tolerance Angle"><FInput value={pattern.tolAngle} onChange={v=>updP('tolAngle',v)} type="number" unit="°" min={0} max={360}/></PropRow>
      <PropRow label="Max Overlap"><FInput value={pattern.maxOverlap} onChange={v=>updP('maxOverlap',v)} type="number" step={0.01} min={0} max={1}/></PropRow>
      <PropRow label="Pick X"><FInput value={pattern.pickX} onChange={v=>updP('pickX',v)} type="number" unit="px"/></PropRow>
      <PropRow label="Pick Y"><FInput value={pattern.pickY} onChange={v=>updP('pickY',v)} type="number" unit="px"/></PropRow>

      {/* EdgeMatchConfig */}
      <div style={{ padding:'4px 10px',background:C.hd,borderBottom:`1px solid #252526`,borderTop:`1px solid ${C.bd}`,marginTop:2 }}>
        <span style={{ fontSize:9,fontWeight:700,color:C.txt3,textTransform:'uppercase',letterSpacing:'0.1em' }}>Edge Match Config</span>
      </div>
      <PropRow label="Thresh Lower"><FInput value={pattern.edge.threshLower} onChange={v=>updE('threshLower',v)} type="number"/></PropRow>
      <PropRow label="Thresh Upper"><FInput value={pattern.edge.threshUpper} onChange={v=>updE('threshUpper',v)} type="number"/></PropRow>
      <PropRow label="Kernel Size">
        <select value={pattern.edge.kernelSize} onChange={e=>updE('kernelSize',parseInt(e.target.value))} style={{ ...fi,padding:'2px 6px',fontSize:11 }}>
          {[1,3,5,7].map(k=><option key={k}>{k}</option>)}
        </select>
      </PropRow>
      <PropRow label="Blur W × H">
        <div style={{ display:'flex',gap:4 }}>
          <FInput value={pattern.edge.blurW} onChange={v=>updE('blurW',v)} type="number" min={1}/>
          <FInput value={pattern.edge.blurH} onChange={v=>updE('blurH',v)} type="number" min={1}/>
        </div>
      </PropRow>
      <PropRow label="Greediness"><FInput value={pattern.edge.greediness} onChange={v=>updE('greediness',v)} type="number" step={0.01} min={0} max={1}/></PropRow>
      <PropRow label="Min Reduce Len"><FInput value={pattern.edge.minReduceLength} onChange={v=>updE('minReduceLength',v)} type="number" unit="px"/></PropRow>
      <PropRow label="T Samples"><FInput value={pattern.edge.tSamples} onChange={v=>updE('tSamples',v)} type="number" min={1}/></PropRow>
      <PropRow label="Invert Binary"><FToggle value={pattern.edge.invertBinary} onChange={v=>updE('invertBinary',v)}/></PropRow>
      <PropRow label="Sub-pixel"><FToggle value={pattern.edge.subPixel} onChange={v=>updE('subPixel',v)}/></PropRow>
      <PropRow label="Stop at Layer 1"><FToggle value={pattern.edge.stopAtLayer1} onChange={v=>updE('stopAtLayer1',v)}/></PropRow>
    </div>
  );
}

/* ─── PANE 2: Library + Setting below ──────────────── */
function LibraryPane({ groups, activeGroupId, sel, onSelectGroup, onSelectPattern,
  onSetActive, onAddGroup, onDeleteGroup, onRenameGroup,
  onAddPattern, onDeletePattern, onRenamePattern,
  onUpdatePattern, onLearnPattern, onEditPattern }) {

  const [openGrps,setOpenGrps]=useState(()=>{ const o={}; groups.forEach(g=>o[g.id]=true); return o; });
  useEffect(()=>setOpenGrps(p=>{ const o={...p}; groups.forEach(g=>{ if(!(g.id in o)) o[g.id]=true; }); return o; }), [groups]);
  const toggle=id=>setOpenGrps(p=>({...p,[id]:!p[id]}));
  const total=groups.reduce((s,g)=>s+g.patterns.length,0);

  return (
    <div style={{ display:'flex',flexDirection:'column',height:'100%',overflow:'hidden' }}>
      {/* Library header */}
      <div style={{ padding:'5px 10px',background:C.hd,borderBottom:`1px solid ${C.bd}`,display:'flex',alignItems:'center',justifyContent:'space-between',flexShrink:0 }}>
        <span style={{ fontSize:9,fontWeight:700,color:C.txt3,textTransform:'uppercase',letterSpacing:'0.12em' }}>Pattern Library</span>
        <span style={{ fontSize:9,color:C.txt3,fontFamily:mono }}>{groups.length}G · {total}P</span>
      </div>

      {/* Library toolbar */}
      <div style={{ padding:'4px 8px',background:C.surf,borderBottom:`1px solid ${C.bd}`,display:'flex',gap:4,flexShrink:0 }}>
        <TBtn onClick={onAddGroup}>
          <svg width="10" height="10" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5" strokeLinecap="round"><line x1="12" y1="5" x2="12" y2="19"/><line x1="5" y1="12" x2="19" y2="12"/></svg>Group
        </TBtn>
        <TBtn disabled={!sel} onClick={()=>sel&&onAddPattern(sel.groupId)}>
          <svg width="10" height="10" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5" strokeLinecap="round"><line x1="12" y1="5" x2="12" y2="19"/><line x1="5" y1="12" x2="19" y2="12"/></svg>Pattern
        </TBtn>
        <div style={{ flex:1 }}/>
        {/* legend */}
        <div style={{ display:'flex',alignItems:'center',gap:5 }}>
          <NumBadge n="N" color={C.warn} size={13} title="Group # — runtime selector"/>
          <NumBadge n="N" color="#9cdcfe" size={13} title="Pattern # — output ID"/>
        </div>
      </div>

      {/* Tree */}
      <div style={{ flex:1,overflowY:'auto',minHeight:0 }}>
        {groups.length===0&&<div style={{ padding:'20px',textAlign:'center',color:C.txt3,fontSize:11 }}>No groups. <span style={{ color:C.acc,cursor:'pointer' }} onClick={onAddGroup}>Add one</span></div>}
        {groups.map(group=>{
          const isOpen=openGrps[group.id]!==false;
          const isGSel=sel?.groupId===group.id&&!sel?.patternId;
          const isActive=activeGroupId===group.id;
          return (
            <div key={group.id}>
              {/* Group row */}
              <div onClick={()=>{ onSelectGroup(group.id); toggle(group.id); }}
                style={{ display:'flex',alignItems:'center',gap:5,padding:'5px 8px 5px 8px',cursor:'pointer',userSelect:'none',
                  background:isGSel?`${C.acc}18`:'transparent',
                  borderLeft:`2px solid ${isGSel?C.acc:'transparent'}`,
                  borderBottom:`1px solid ${C.bd}`,transition:'background 0.1s',
                }}
                onMouseEnter={e=>{ if(!isGSel) e.currentTarget.style.background=C.surf2; }}
                onMouseLeave={e=>{ if(!isGSel) e.currentTarget.style.background='transparent'; }}>
                <svg width="10" height="10" viewBox="0 0 24 24" fill="none" stroke={C.txt3} strokeWidth="2.5" strokeLinecap="round"
                  onClick={e=>{e.stopPropagation();toggle(group.id);}}
                  style={{ transform:isOpen?'rotate(90deg)':'none',transition:'transform 0.14s',flexShrink:0 }}>
                  <polyline points="9 18 15 12 9 6"/>
                </svg>
                <NumBadge n={group.number} color={C.warn} size={18} title={`Runtime: activate group by sending #${group.number}`}/>
                <IcoFolder size={12} color={isGSel?C.acc:C.txt2}/>
                <div style={{ flex:1,minWidth:0 }} onClick={e=>e.stopPropagation()}>
                  <InlineEdit value={group.name} onCommit={v=>onRenameGroup(group.id,v)}/>
                </div>
                <span style={{ fontSize:9,color:C.txt3,fontFamily:mono }}>{group.patterns.length}p</span>
                {isActive&&<div style={{ width:6,height:6,borderRadius:'50%',background:C.acc,boxShadow:`0 0 5px ${C.acc}`,flexShrink:0 }} title="Active"/>}
                <div onClick={e=>e.stopPropagation()} style={{ display:'flex',gap:1 }}>
                  <IBtn title="Set active" icon={<svg width="11" height="11" viewBox="0 0 24 24" fill="none" stroke={isActive?C.acc:'currentColor'} strokeWidth="2" strokeLinecap="round"><polygon points="13 2 3 14 12 14 11 22 21 10 12 10 13 2"/></svg>} onClick={()=>onSetActive(group.id)}/>
                  <IBtn title="Delete group" danger icon={<svg width="11" height="11" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.8" strokeLinecap="round" strokeLinejoin="round"><polyline points="3 6 5 6 21 6"/><path d="M19 6l-1 14a2 2 0 0 1-2 2H8a2 2 0 0 1-2-2L5 6"/></svg>} onClick={()=>onDeleteGroup(group.id)}/>
                </div>
              </div>

              {/* Pattern rows */}
              {isOpen&&<div style={{ paddingLeft:20 }}>
                {group.patterns.length===0?(
                  <div onClick={()=>onAddPattern(group.id)} style={{ padding:'4px 10px',fontSize:10,color:C.txt3,cursor:'pointer',display:'flex',alignItems:'center',gap:4,borderBottom:`1px solid ${C.bd}` }}
                    onMouseEnter={e=>e.currentTarget.style.color=C.acc} onMouseLeave={e=>e.currentTarget.style.color=C.txt3}>
                    <svg width="10" height="10" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5" strokeLinecap="round"><line x1="12" y1="5" x2="12" y2="19"/><line x1="5" y1="12" x2="19" y2="12"/></svg>Add pattern
                  </div>
                ):group.patterns.map(pat=>{
                  const isPS=sel?.groupId===group.id&&sel?.patternId===pat.id;
                  return (
                    <div key={pat.id} onClick={()=>onSelectPattern(group.id,pat.id)}
                      style={{ display:'flex',alignItems:'center',gap:5,padding:'4px 8px 4px 6px',cursor:'pointer',
                        background:isPS?`${C.acc}15`:'transparent',
                        borderLeft:`2px solid ${isPS?'#9cdcfe':'transparent'}`,
                        borderBottom:`1px solid ${C.bd}`,transition:'background 0.1s',
                      }}
                      onMouseEnter={e=>{ if(!isPS) e.currentTarget.style.background=C.surf2; }}
                      onMouseLeave={e=>{ if(!isPS) e.currentTarget.style.background='transparent'; }}>
                      <Dot ok={pat.learned}/>
                      <NumBadge n={pat.number} color="#9cdcfe" size={15} title={`Output ID: ${pat.number}`}/>
                      <div style={{ flex:1,minWidth:0 }} onClick={e=>e.stopPropagation()}>
                        <InlineEdit value={pat.name} onCommit={v=>onRenamePattern(group.id,pat.id,v)}/>
                      </div>
                      <span style={{ fontSize:9,color:pat.minScore>=0.9?C.ok:C.warn,fontFamily:mono,flexShrink:0 }}>{pat.minScore.toFixed(2)}</span>
                      <div onClick={e=>e.stopPropagation()} style={{ display:'flex',gap:1 }}>
                        <IBtn title="Edit pattern" icon={<svg width="11" height="11" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.8" strokeLinecap="round"><path d="M11 4H4a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h14a2 2 0 0 0 2-2v-7"/><path d="M18.5 2.5a2.121 2.121 0 0 1 3 3L12 15l-4 1 1-4 9.5-9.5z"/></svg>} onClick={()=>onEditPattern(group.id,pat.id)}/>
                        <IBtn danger title="Delete pattern" icon={<svg width="11" height="11" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.8" strokeLinecap="round" strokeLinejoin="round"><polyline points="3 6 5 6 21 6"/><path d="M19 6l-1 14a2 2 0 0 1-2 2H8a2 2 0 0 1-2-2L5 6"/></svg>} onClick={()=>onDeletePattern(group.id,pat.id)}/>
                      </div>
                    </div>
                  );
                })}
              </div>}
            </div>
          );
        })}
      </div>

    </div>
  );
}

/* ─── PANE 3: Property Inspector ────────────────────── */
function PMPropertyPane({ sel, groups }) {
  const group   = sel ? groups.find(g=>g.id===sel.groupId) : null;
  const pattern = group&&sel?.patternId ? group.patterns.find(p=>p.id===sel.patternId) : null;

  // Build schema from selected context
  const schema = group ? (pattern ? [
    { group:'Pattern Identity', open:true, props:[
      { key:'name',    label:'Name',           type:'readonly', value:pattern.name },
      { key:'number',  label:'Pattern Number', type:'readonly', value:pattern.number },
      { key:'learned', label:'Learned',        type:'readonly', value:pattern.learned?'Yes':'No' },
      { key:'type',    label:'Match Type',     type:'readonly', value:pattern.matchType },
    ]},
    { group:'Match Parameters', open:true, props:[
      { key:'minScore',  label:'Min Score',       type:'number', value:pattern.minScore,  unit:'0–1',  step:0.01 },
      { key:'angle',     label:'Angle',           type:'number', value:pattern.angle,     unit:'°'              },
      { key:'tolAngle',  label:'Tolerance Angle', type:'number', value:pattern.tolAngle,  unit:'°'              },
      { key:'maxOverlap',label:'Max Overlap',     type:'number', value:pattern.maxOverlap,unit:'0–1',  step:0.01 },
      { key:'pickX',     label:'Pick Position X', type:'number', value:pattern.pickX,     unit:'px'             },
      { key:'pickY',     label:'Pick Position Y', type:'number', value:pattern.pickY,     unit:'px'             },
    ]},
    { group:'Edge Match Config', open:false, props:[
      { key:'threshLower',      label:'Thresh Lower',     type:'number', value:pattern.edge.threshLower  },
      { key:'threshUpper',      label:'Thresh Upper',     type:'number', value:pattern.edge.threshUpper  },
      { key:'kernelSize',       label:'Kernel Size',      type:'select', value:pattern.edge.kernelSize, options:[1,3,5,7] },
      { key:'blurW',            label:'Blur Width',       type:'number', value:pattern.edge.blurW,       unit:'px' },
      { key:'blurH',            label:'Blur Height',      type:'number', value:pattern.edge.blurH,       unit:'px' },
      { key:'greediness',       label:'Greediness',       type:'number', value:pattern.edge.greediness,  step:0.01 },
      { key:'minReduceLength',  label:'Min Reduce Len',   type:'number', value:pattern.edge.minReduceLength, unit:'px' },
      { key:'tSamples',         label:'T Samples',        type:'number', value:pattern.edge.tSamples             },
      { key:'invertBinary',     label:'Invert Binary',    type:'bool',   value:pattern.edge.invertBinary  },
      { key:'subPixel',         label:'Sub-pixel',        type:'bool',   value:pattern.edge.subPixel      },
      { key:'stopAtLayer1',     label:'Stop at Layer 1',  type:'bool',   value:pattern.edge.stopAtLayer1  },
    ]},
  ] : [
    { group:`Group #${group.number}`, open:true, props:[
      { key:'name',   label:'Name',                 type:'readonly', value:group.name   },
      { key:'number', label:'Group Number',          type:'readonly', value:group.number },
    ]},
    { group:'Group Config', open:true, props:[
      { key:'lowWorkpieceRatio', label:'Low Workpiece Ratio', type:'number', value:group.lowWorkpieceRatio, step:0.1 },
      { key:'pickBoxW',  label:'Picking Box W',      type:'number', value:group.pickBoxW,   unit:'px' },
      { key:'pickBoxH',  label:'Picking Box H',      type:'number', value:group.pickBoxH,   unit:'px' },
      { key:'pickBoxDist',label:'Box Distance',      type:'number', value:group.pickBoxDist,unit:'px' },
      { key:'pickBoxAngle',label:'Box Angle',        type:'number', value:group.pickBoxAngle,unit:'°' },
    ]},
  ]) : [];

  const [focused,setFocused]=useState(null);
  const [search,setSearch]=useState('');

  const filtered = search.trim()
    ? schema.map(g=>({...g,open:true,props:g.props.filter(p=>p.label.toLowerCase().includes(search.toLowerCase()))})).filter(g=>g.props.length>0)
    : schema;

  return (
    <div style={{ display:'flex',flexDirection:'column',height:'100%',overflow:'hidden' }}>
      {/* Header */}
      <div style={{ padding:'5px 10px',background:C.hd,borderBottom:`1px solid ${C.bd}`,display:'flex',alignItems:'center',gap:8,flexShrink:0 }}>
        <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke={C.txt3} strokeWidth="2" strokeLinecap="round"><line x1="3" y1="6" x2="21" y2="6"/><line x1="3" y1="12" x2="21" y2="12"/><line x1="3" y1="18" x2="21" y2="18"/></svg>
        <span style={{ fontSize:9,fontWeight:700,color:C.txt3,textTransform:'uppercase',letterSpacing:'0.12em',flex:1 }}>Properties</span>
      </div>
      {/* Context */}
      {group&&<div style={{ padding:'4px 10px',background:'#252526',borderBottom:`1px solid ${C.bd}`,display:'flex',alignItems:'center',gap:6 }}>
        <div style={{ width:5,height:5,borderRadius:'50%',background:pattern?'#9cdcfe':C.warn,flexShrink:0 }}/>
        <span style={{ fontSize:10,fontFamily:mono,color:C.txt3,overflow:'hidden',textOverflow:'ellipsis',whiteSpace:'nowrap' }}>
          {pattern?`${group.name} › ${pattern.name}`:group.name}
        </span>
      </div>}
      {/* Search */}
      <div style={{ padding:'5px 8px',borderBottom:`1px solid ${C.bd}`,flexShrink:0 }}>
        <div style={{ position:'relative' }}>
          <svg width="10" height="10" viewBox="0 0 24 24" fill="none" stroke={C.txt3} strokeWidth="2.5" strokeLinecap="round" style={{ position:'absolute',left:7,top:'50%',transform:'translateY(-50%)' }}>
            <circle cx="11" cy="11" r="8"/><path d="M21 21l-4.35-4.35"/>
          </svg>
          <input value={search} onChange={e=>setSearch(e.target.value)} placeholder="Filter…"
            style={{ ...fi,padding:'4px 8px 4px 22px',fontSize:10,border:`1px solid ${C.bd}` }}
            onFocus={e=>e.target.style.borderColor=C.acc} onBlur={e=>e.target.style.borderColor=C.bd}/>
        </div>
      </div>
      {/* Props */}
      {schema.length===0?(
        <div style={{ flex:1,display:'flex',alignItems:'center',justifyContent:'center',color:C.txt3,fontSize:11,padding:12,textAlign:'center' }}>
          Select a group or pattern<br/>from the library
        </div>
      ):(
        <div style={{ flex:1,overflowY:'auto' }}>
          {filtered.map(grp=>{
            const [open,setOpen]=useState(grp.open);
            return (
              <div key={grp.group}>
                <div onClick={()=>setOpen(o=>!o)} style={{ display:'flex',alignItems:'center',gap:5,padding:'4px 10px',background:C.hd,borderBottom:`1px solid #252526`,cursor:'pointer' }}
                  onMouseEnter={e=>e.currentTarget.style.background='#2a2d2e'}
                  onMouseLeave={e=>e.currentTarget.style.background=C.hd}>
                  <svg width="9" height="9" viewBox="0 0 24 24" fill="none" stroke={C.txt3} strokeWidth="2.5" strokeLinecap="round"
                    style={{ transform:open?'rotate(90deg)':'none',transition:'transform 0.14s',flexShrink:0 }}>
                    <polyline points="9 18 15 12 9 6"/>
                  </svg>
                  <span style={{ fontSize:9,fontWeight:700,color:C.txt3,textTransform:'uppercase',letterSpacing:'0.1em',flex:1 }}>{grp.group}</span>
                </div>
                {open&&grp.props.map(prop=>(
                  <div key={prop.key}
                    style={{ display:'grid',gridTemplateColumns:'1fr 1fr',alignItems:'center',
                      padding:'3px 10px 3px 12px',borderBottom:`1px solid #1e1e1e`,minHeight:26,
                      background:focused===prop.key?'#094771':'transparent',
                    }}>
                    <span style={{ fontSize:11,color:'#7a7a7a',fontWeight:500,overflow:'hidden',textOverflow:'ellipsis',whiteSpace:'nowrap' }} title={prop.label}>{prop.label}</span>
                    <div>
                      {prop.type==='readonly'&&<span style={{ fontSize:11,fontFamily:mono,color:C.txt3,padding:'2px 6px' }}>{prop.value}</span>}
                      {prop.type==='bool'&&<div onClick={()=>{}} style={{ width:28,height:15,borderRadius:8,cursor:'pointer',background:prop.value?C.acc:'#3c3c3c',position:'relative',border:`1px solid ${prop.value?C.acc:C.bd2}` }}>
                        <div style={{ position:'absolute',top:2,left:prop.value?14:2,width:9,height:9,borderRadius:'50%',background:'#fff' }}/>
                      </div>}
                      {prop.type==='number'&&(
                        <div style={{ display:'flex',alignItems:'center',gap:3 }}>
                          <input type="number" value={prop.value} readOnly
                            onFocus={()=>setFocused(prop.key)} onBlur={()=>setFocused(null)}
                            style={{ ...fi,padding:'2px 5px',fontSize:11,flex:1,borderColor:focused===prop.key?C.acc:C.bd }}/>
                          {prop.unit&&<span style={{ fontSize:9,color:C.txt3,fontFamily:mono }}>{prop.unit}</span>}
                        </div>
                      )}
                      {prop.type==='select'&&<select value={prop.value} style={{ ...fi,padding:'2px 5px',fontSize:11 }}>
                        {prop.options.map(o=><option key={o}>{o}</option>)}
                      </select>}
                    </div>
                  </div>
                ))}
              </div>
            );
          })}
        </div>
      )}
      {/* Footer hint */}
      <div style={{ padding:'6px 12px',borderTop:`1px solid ${C.bd}`,background:'#252526',flexShrink:0 }}>
        <div style={{ fontSize:9,color:C.txt3,lineHeight:1.5 }}>
          {focused
            ? <span style={{ color:C.acc,fontFamily:mono }}>{focused}</span>
            : 'Click property to see details'}
        </div>
      </div>
    </div>
  );
}

/* ─── ROOT ───────────────────────────────────────────── */
function PatternManagerPanel() {
  const [groups,        setGroups]       = useState(INIT_GROUPS);
  const [activeGroupId, setActiveGrpId]  = useState(INIT_GROUPS[0]?.id||null);
  const [sel,           setSel]          = useState(null);
  const [dlg,           setDlg]          = useState(null);
  const [resultTableH,  setResultTableH] = useState(140);

  const addGroup    = (name,number) => { const g=mkGrp(name,number); setGroups(p=>[...p,g]); setSel({groupId:g.id}); setDlg(null); };
  const deleteGroup = id => { setGroups(p=>p.filter(g=>g.id!==id)); if(sel?.groupId===id) setSel(null); setDlg(null); };
  const renameGroup = (id,name) => setGroups(p=>p.map(g=>g.id===id?{...g,name}:g));

  const addPattern    = (gId,name,number,extra={}) => { const p={...mkPat(name,number,!!extra.image),...extra}; setGroups(prev=>prev.map(g=>g.id===gId?{...g,patterns:[...g.patterns,p]}:g)); setSel({groupId:gId,patternId:p.id}); setDlg(null); };
  const deletePattern = (gId,pId) => { setGroups(prev=>prev.map(g=>g.id===gId?{...g,patterns:g.patterns.filter(p=>p.id!==pId)}:g)); if(sel?.patternId===pId) setSel({groupId:gId}); };
  const renamePattern = (gId,pId,name) => setGroups(prev=>prev.map(g=>g.id===gId?{...g,patterns:g.patterns.map(p=>p.id===pId?{...p,name}:p)}:g));
  const updatePattern = (gId,pId,key,val) => {
    setGroups(prev=>prev.map(g=>g.id===gId?{...g,patterns:g.patterns.map(p=>{
      if(p.id!==pId) return p;
      if(key.startsWith('edge.')) return {...p,edge:{...p.edge,[key.slice(5)]:val}};
      return {...p,[key]:val};
    })}:g));
  };
  const learnPattern = (gId,pId) => setGroups(prev=>prev.map(g=>g.id===gId?{...g,patterns:g.patterns.map(p=>p.id===pId?{...p,learned:true}:p)}:g));

  const editPatternFromWizard = (gId, pId, updates) => {
    setGroups(prev=>prev.map(g=>g.id===gId ? {
      ...g,
      pickBoxW:    updates.pickBoxW,
      pickBoxH:    updates.pickBoxH,
      pickBoxDist: updates.pickBoxDist,
      pickBoxAngle:updates.pickBoxAngle,
      patterns: g.patterns.map(p=>p.id===pId ? {
        ...p,
        name:   updates.name,
        number: updates.number,
        pickX:  updates.pickX,
        pickY:  updates.pickY,
        boxA: { w:updates.pickBoxW, h:updates.pickBoxH, dist:updates.pickBoxDist, angle:updates.pickBoxAngle },
        boxB: { w:updates.pickBoxW, h:updates.pickBoxH, dist:updates.pickBoxDist, angle:updates.pickBoxAngle+180 },
      } : p),
    } : g));
    setSel({groupId:gId, patternId:pId});
    setDlg(null);
  };

  const usedGroupNums  = groups.map(g=>g.number);
  const usedGroupNames = groups.map(g=>g.name);
  const dlgGrp         = dlg?.groupId ? groups.find(g=>g.id===dlg.groupId) : null;
  const dlgPat         = dlgGrp && dlg?.patternId ? dlgGrp.patterns.find(p=>p.id===dlg.patternId) : null;

  return (
    <div style={{ display:'flex',height:'100%',overflow:'hidden',background:C.bg,fontFamily:sans,color:C.txt }}>
      {/* Pane 1: Monitor + Result Table */}
      <div style={{ flex:1,display:'flex',flexDirection:'column',overflow:'hidden',borderRight:`1px solid ${C.bd}`,minWidth:0 }}>
        <MonitorPane groups={groups} activeGroupId={activeGroupId} onSetActive={setActiveGrpId}
          resultTableH={resultTableH} onResizeResultTable={setResultTableH}/>
      </div>

      {/* Pane 2: Library tree + Pattern/Group Properties */}
      <div style={{ width:320,flexShrink:0,overflow:'hidden',display:'flex',flexDirection:'column' }}>
        <LibraryPane
          groups={groups} activeGroupId={activeGroupId} sel={sel}
          onSelectGroup={id=>setSel({groupId:id})}
          onSelectPattern={(gId,pId)=>setSel({groupId:gId,patternId:pId})}
          onSetActive={setActiveGrpId}
          onAddGroup={()=>setDlg({type:'addGroup'})}
          onDeleteGroup={deleteGroup}
          onRenameGroup={renameGroup}
          onAddPattern={id=>setDlg({type:'addPattern',groupId:id})}
          onDeletePattern={deletePattern}
          onRenamePattern={renamePattern}
          onUpdatePattern={updatePattern}
          onLearnPattern={learnPattern}
          onEditPattern={(gId,pId)=>setDlg({type:'editPattern',groupId:gId,patternId:pId})}
        />
      </div>

      {/* Dialogs */}
      {dlg?.type==='addGroup'&&<AddGroupDlg usedNums={usedGroupNums} usedNames={usedGroupNames} onConfirm={addGroup} onClose={()=>setDlg(null)}/>}
      {dlg?.type==='addPattern'&&dlgGrp&&window.PatternWizard&&<window.PatternWizard groupName={dlgGrp.name} usedNums={dlgGrp.patterns.map(p=>p.number)} usedNames={dlgGrp.patterns.map(p=>p.name)} onConfirm={d=>addPattern(dlgGrp.id,d.name,d.number,{image:d.image,crop:d.crop,pickPos:d.pickPos,boxA:d.boxA,boxB:d.boxB})} onClose={()=>setDlg(null)}/>}
      {dlg?.type==='editPattern'&&dlgGrp&&dlgPat&&window.EditPatternWizard&&(
        <window.EditPatternWizard
          groupName={dlgGrp.name}
          pattern={{
            ...dlgPat,
            pickBoxW:    dlgPat.boxA?.w     ?? dlgGrp.pickBoxW,
            pickBoxH:    dlgPat.boxA?.h     ?? dlgGrp.pickBoxH,
            pickBoxDist: dlgPat.boxA?.dist  ?? dlgGrp.pickBoxDist,
            pickBoxAngle:dlgPat.boxA?.angle ?? dlgGrp.pickBoxAngle,
          }}
          usedNums={dlgGrp.patterns.filter(p=>p.id!==dlgPat.id).map(p=>p.number)}
          usedNames={dlgGrp.patterns.filter(p=>p.id!==dlgPat.id).map(p=>p.name)}
          onConfirm={d=>editPatternFromWizard(dlgGrp.id,dlgPat.id,d)}
          onClose={()=>setDlg(null)}
        />
      )}
    </div>
  );
}

Object.assign(window, { PatternManagerPanel });
