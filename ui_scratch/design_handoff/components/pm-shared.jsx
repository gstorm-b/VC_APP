
// pm-shared.jsx — Shared tokens, tiny components, dialogs for Pattern Manager
const { useState, useRef, useEffect } = React;

/* ── tokens ── */
const PMT = {
  bg:'#060d18', surf:'#0b1524', surf2:'#0f1d2e',
  bd:'#1a2a3a', bd2:'#243444',
  tx:'#dce8f5', tx2:'#4a6080', tx3:'#1f2d42',
  acc:'#2b8ce8', accBg:'#2b8ce815',
  ok:'#22d17a', warn:'#f5a623', err:'#e84040',
  hd:'#060d18',
};
window.PMT = PMT;

/* ── seed data ── */
let _pmid = 10;
const pmid = () => `id${_pmid++}`;

function makeEdge() {
  return { threshLower:100, threshUpper:200, kernelSize:3, blurW:5, blurH:5, greediness:0, minReduceLength:32, tSamples:3, invertBin:true, subPixel:false, stopAt1:false };
}
function makePattern(name, number, learned=true) {
  return { id:pmid(), name, number, learned, minScore:0.75, angle:0, tolAngle:180, maxOverlap:0.1, pickX:0, pickY:0, matchType:'EdgeBased', edge:makeEdge() };
}
function makeGroup(name, number, patterns=[]) {
  return { id:pmid(), name, number, lowRatio:1.5, boxW:50, boxH:50, boxDist:0, boxAngle:0, patterns };
}

window.pmMakeEdge    = makeEdge;
window.pmMakePattern = makePattern;
window.pmMakeGroup   = makeGroup;
window.INIT_PM_GROUPS = [
  makeGroup('Part_A', 1, [ makePattern('Front_face',1,true), makePattern('Side_face',2,true), makePattern('Top_view',3,false) ]),
  makeGroup('Part_B', 2, [ makePattern('Body_01',1,true), makePattern('Logo_01',2,false) ]),
  makeGroup('Part_C', 3, []),
];

/* ── TBtn ── */
function TBtn({ children, onClick, title, active, small, disabled, accent }) {
  const [h,setH] = useState(false);
  const bg = active ? PMT.accBg : h ? PMT.surf2 : 'transparent';
  const bd = active ? PMT.acc   : h ? PMT.bd2   : PMT.bd;
  const cl = disabled ? PMT.tx3 : active ? PMT.acc : h ? PMT.tx : PMT.tx2;
  return (
    <button onClick={disabled?null:onClick} title={title} style={{
      background: disabled ? 'transparent' : bg,
      border:`1px solid ${disabled?PMT.bd:bd}`,
      borderRadius:4, padding:small?'3px 8px':'5px 12px',
      color:cl, fontSize:small?10:11, fontWeight:600, cursor:disabled?'default':'pointer',
      display:'flex', alignItems:'center', gap:5,
      fontFamily:'Space Grotesk, sans-serif', transition:'all 0.12s', flexShrink:0,
    }}
      onMouseEnter={()=>!disabled&&setH(true)}
      onMouseLeave={()=>setH(false)}
    >{children}</button>
  );
}
window.TBtn = TBtn;

/* ── IconBtn ── */
function PMIconBtn({ icon, title, onClick, danger }) {
  const [h,setH] = useState(false);
  return (
    <button onClick={onClick} title={title} style={{
      background:h?(danger?'#2a1a1a':PMT.surf2):'transparent',
      border:'none', borderRadius:4, padding:'3px 5px', cursor:'pointer',
      color:h?(danger?PMT.err:PMT.tx2):PMT.tx3, display:'flex', alignItems:'center',
    }}
      onMouseEnter={()=>setH(true)} onMouseLeave={()=>setH(false)}
    >{icon}</button>
  );
}
window.PMIconBtn = PMIconBtn;

/* ── SectionHd ── */
function SectionHd({ label, count, action }) {
  return (
    <div style={{ height:26, background:PMT.hd, borderBottom:`1px solid ${PMT.bd}`, display:'flex', alignItems:'center', padding:'0 10px', gap:8, flexShrink:0 }}>
      <span style={{ fontSize:9, fontWeight:700, color:PMT.tx3, textTransform:'uppercase', letterSpacing:'0.12em', flex:1 }}>{label}</span>
      {count!==undefined && <span style={{ fontSize:9, color:PMT.tx3, fontFamily:'JetBrains Mono, monospace' }}>{count}</span>}
      {action}
    </div>
  );
}
window.SectionHd = SectionHd;

/* ── Inline rename ── */
function InlineEdit({ value, onCommit }) {
  const [v, setV] = useState(value);
  const ref = useRef(null);
  useEffect(()=>{ ref.current?.select(); },[]);
  const commit = () => onCommit(v.trim()||value);
  return (
    <input ref={ref} value={v} onChange={e=>setV(e.target.value)}
      onKeyDown={e=>{ if(e.key==='Enter') commit(); if(e.key==='Escape') onCommit(value); }}
      onBlur={commit} autoFocus
      style={{ background:PMT.bg, border:`1px solid ${PMT.acc}`, borderRadius:3, padding:'1px 5px', color:PMT.tx, fontSize:11, fontFamily:'JetBrains Mono, monospace', outline:'none', width:'100%', boxSizing:'border-box' }}
    />
  );
}
window.InlineEdit = InlineEdit;

/* ── Modal wrapper ── */
function PMModal({ title, subtitle, onClose, width=420, children }) {
  return (
    <div onClick={onClose} style={{ position:'fixed', inset:0, background:'rgba(5,9,18,0.8)', display:'flex', alignItems:'center', justifyContent:'center', zIndex:1000, backdropFilter:'blur(3px)' }}>
      <div onClick={e=>e.stopPropagation()} style={{ width, background:'#111827', border:`1px solid ${PMT.bd2}`, borderRadius:9, overflow:'hidden', boxShadow:'0 24px 64px rgba(0,0,0,0.6)' }}>
        <div style={{ padding:'12px 18px', background:PMT.hd, borderBottom:`1px solid ${PMT.bd}`, display:'flex', alignItems:'center', justifyContent:'space-between' }}>
          <div>
            <div style={{ fontSize:13, fontWeight:700, color:PMT.tx }}>{title}</div>
            {subtitle && <div style={{ fontSize:10, color:PMT.tx3, marginTop:2 }}>{subtitle}</div>}
          </div>
          <button onClick={onClose} style={{ background:'transparent', border:'none', cursor:'pointer', color:PMT.tx3 }}>
            <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.2" strokeLinecap="round"><line x1="18" y1="6" x2="6" y2="18"/><line x1="6" y1="6" x2="18" y2="18"/></svg>
          </button>
        </div>
        <div style={{ padding:'20px 20px' }}>{children}</div>
      </div>
    </div>
  );
}
window.PMModal = PMModal;

const dlgInp = { background:PMT.bg, border:`1px solid ${PMT.bd2}`, borderRadius:4, padding:'7px 10px', color:PMT.tx, fontSize:12, fontFamily:'JetBrains Mono, monospace', outline:'none', width:'100%', boxSizing:'border-box' };
window.dlgInp = dlgInp;

/* ── AddGroupDialog ── */
function AddGroupDialog({ usedNums, usedNames, onConfirm, onClose }) {
  const [name, setName] = useState('');
  const [num,  setNum]  = useState('');
  const numInt = parseInt(num) || 0;
  const nameErr = name.trim() && usedNames.includes(name.trim()) ? 'Name already exists' : null;
  const numErr  = num && (isNaN(numInt) || numInt<1 || usedNums.includes(numInt)) ? (usedNums.includes(numInt)?'Number already exists':'Must be ≥ 1') : null;
  const valid = name.trim() && num && !nameErr && !numErr;
  return (
    <PMModal title="New Pattern Group" onClose={onClose}>
      <div style={{ display:'flex', flexDirection:'column', gap:14 }}>
        <div>
          <label style={{ fontSize:10, fontWeight:700, color:PMT.tx2, textTransform:'uppercase', letterSpacing:'0.08em', display:'block', marginBottom:5 }}>Group Name</label>
          <input style={{ ...dlgInp, borderColor:nameErr?PMT.err:PMT.bd2 }} placeholder="e.g. Part_A" value={name} onChange={e=>setName(e.target.value)} autoFocus onFocus={e=>e.target.style.borderColor=PMT.acc} onBlur={e=>e.target.style.borderColor=nameErr?PMT.err:PMT.bd2}/>
          {nameErr && <div style={{ fontSize:10, color:PMT.err, marginTop:3 }}>{nameErr}</div>}
        </div>
        <div>
          <label style={{ fontSize:10, fontWeight:700, color:PMT.tx2, textTransform:'uppercase', letterSpacing:'0.08em', display:'block', marginBottom:5 }}>Group Number</label>
          <input style={{ ...dlgInp, borderColor:numErr?PMT.err:PMT.bd2 }} type="number" min="1" placeholder="Unique integer" value={num} onChange={e=>setNum(e.target.value)} onFocus={e=>e.target.style.borderColor=PMT.acc} onBlur={e=>e.target.style.borderColor=numErr?PMT.err:PMT.bd2}/>
          {numErr ? <div style={{ fontSize:10, color:PMT.err, marginTop:3 }}>{numErr}</div>
            : <div style={{ fontSize:10, color:PMT.tx3, marginTop:3 }}>Runtime selector — PLC sends this number to activate group</div>}
        </div>
        <div style={{ display:'flex', gap:8, justifyContent:'flex-end' }}>
          <TBtn onClick={onClose}>Cancel</TBtn>
          <button onClick={()=>valid&&onConfirm(name.trim(), numInt)} style={{ background:valid?PMT.acc:PMT.surf2, border:'none', borderRadius:5, padding:'8px 20px', fontSize:12, fontWeight:600, color:valid?'#fff':PMT.tx3, cursor:valid?'pointer':'default' }}>Create Group</button>
        </div>
      </div>
    </PMModal>
  );
}
window.AddGroupDialog = AddGroupDialog;

/* ── AddPatternDialog ── */
function AddPatternDialog({ groupName, groupNum, usedNums, usedNames, onConfirm, onClose }) {
  const [name, setName] = useState('');
  const [num,  setNum]  = useState('');
  const numInt = parseInt(num) || 0;
  const nameErr = name.trim() && usedNames.includes(name.trim()) ? 'Name already exists in group' : null;
  const numErr  = num && (isNaN(numInt)||numInt<1||usedNums.includes(numInt)) ? (usedNums.includes(numInt)?'Number already exists in group':'Must be ≥ 1') : null;
  const valid = name.trim() && num && !nameErr && !numErr;
  return (
    <PMModal title="New Pattern" subtitle={`Group: ${groupName} [#${groupNum}]`} onClose={onClose} width={440}>
      <div style={{ display:'flex', flexDirection:'column', gap:14 }}>
        <div>
          <label style={{ fontSize:10, fontWeight:700, color:PMT.tx2, textTransform:'uppercase', letterSpacing:'0.08em', display:'block', marginBottom:5 }}>Pattern Name</label>
          <input style={{ ...dlgInp, borderColor:nameErr?PMT.err:PMT.bd2 }} placeholder="e.g. Front_face" value={name} onChange={e=>setName(e.target.value)} autoFocus onFocus={e=>e.target.style.borderColor=PMT.acc} onBlur={e=>e.target.style.borderColor=nameErr?PMT.err:PMT.bd2}/>
          {nameErr && <div style={{ fontSize:10, color:PMT.err, marginTop:3 }}>{nameErr}</div>}
        </div>
        <div>
          <label style={{ fontSize:10, fontWeight:700, color:PMT.tx2, textTransform:'uppercase', letterSpacing:'0.08em', display:'block', marginBottom:5 }}>Pattern Number</label>
          <input style={{ ...dlgInp, borderColor:numErr?PMT.err:PMT.bd2 }} type="number" min="1" placeholder="Unique in group" value={num} onChange={e=>setNum(e.target.value)} onFocus={e=>e.target.style.borderColor=PMT.acc} onBlur={e=>e.target.style.borderColor=numErr?PMT.err:PMT.bd2}/>
          {numErr ? <div style={{ fontSize:10, color:PMT.err, marginTop:3 }}>{numErr}</div>
            : <div style={{ fontSize:10, color:PMT.tx3, marginTop:3 }}>Reported as Pattern ID in PLC output registers</div>}
        </div>
        <div style={{ background:PMT.bg, border:`1px solid ${PMT.bd}`, borderRadius:5, padding:'10px 12px', fontSize:11, color:PMT.tx3, display:'flex', gap:8, alignItems:'center' }}>
          <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.6" strokeLinecap="round" strokeLinejoin="round"><path d="M23 19a2 2 0 0 1-2 2H3a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2h4l2-3h6l2 3h4a2 2 0 0 1 2 2z"/><circle cx="12" cy="13" r="4"/></svg>
          Image & learn can be set after creation via the inspector.
        </div>
        <div style={{ display:'flex', gap:8, justifyContent:'flex-end' }}>
          <TBtn onClick={onClose}>Cancel</TBtn>
          <button onClick={()=>valid&&onConfirm(name.trim(), numInt)} style={{ background:valid?PMT.acc:PMT.surf2, border:'none', borderRadius:5, padding:'8px 20px', fontSize:12, fontWeight:600, color:valid?'#fff':PMT.tx3, cursor:valid?'pointer':'default' }}>Add Pattern</button>
        </div>
      </div>
    </PMModal>
  );
}
window.AddPatternDialog = AddPatternDialog;
