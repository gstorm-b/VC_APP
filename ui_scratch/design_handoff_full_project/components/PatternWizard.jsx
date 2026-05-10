/* ───────────────────────────── PatternWizard.jsx
   5-step wizard: Image → Crop → Pick Point → Picking Box → Finish.
   Drops onto window.PatternWizard for use by PatternManager.jsx.
   ────────────────────────────────────────────────────────── */
const { useState, useRef, useEffect, useMemo } = React;

/* Palette mirrors PatternManager.jsx */
const PWC = {
  bg:'#1e1e1e', surf:'#252526', surf2:'#2d2d2d',
  bd:'#3c3c3c', bd2:'#454545',
  txt:'#cccccc', txt2:'#9d9d9d', txt3:'#6a6a6a',
  acc:'#0e639c', accHi:'#1177bb', ok:'#4ec9b0', warn:'#dcb67a', err:'#f48771',
  hd:'#1e1e1e',
};
const PWmono = 'JetBrains Mono, monospace';
const PWsans = 'Space Grotesk, sans-serif';

/* canvas dims (display px) */
const CW = 560, CH = 380;

/* ═════════════ small helpers ═════════════ */
function PWBtn({ children, onClick, active, primary, disabled, title }) {
  const [h, setH] = useState(false);
  const bg = disabled ? PWC.surf
    : primary ? (h ? PWC.accHi : PWC.acc)
    : active  ? `${PWC.acc}33`
    : h ? PWC.surf2 : 'transparent';
  const color = disabled ? PWC.txt3
    : primary ? '#fff' : active ? PWC.acc : PWC.txt;
  const bd = primary ? 'transparent'
    : active ? PWC.acc
    : h ? PWC.bd2 : PWC.bd;
  return (
    <button
      title={title}
      disabled={disabled}
      onMouseEnter={()=>setH(true)} onMouseLeave={()=>setH(false)}
      onClick={onClick}
      style={{
        background:bg, color, border:`1px solid ${bd}`,
        padding: primary ? '7px 18px' : '5px 12px',
        fontSize:11, fontWeight: primary ? 700 : 500,
        fontFamily:PWsans, borderRadius:4,
        cursor:disabled?'not-allowed':'pointer',
        display:'inline-flex', alignItems:'center', gap:6,
        transition:'all 0.12s', whiteSpace:'nowrap',
      }}>
      {children}
    </button>
  );
}

function PWNum({ value, onChange, min, max, step=1, suffix, w=82 }) {
  const clamp = v => {
    if (Number.isNaN(v)) return min ?? 0;
    if (min !== undefined && v < min) return min;
    if (max !== undefined && v > max) return max;
    return v;
  };
  return (
    <div style={{ display:'flex', alignItems:'stretch', width:w, border:`1px solid ${PWC.bd2}`, borderRadius:4, background:PWC.bg, fontFamily:PWmono }}>
      <input type="number" value={value} onChange={e=>onChange(clamp(parseFloat(e.target.value)))}
        step={step} min={min} max={max}
        style={{ flex:1, minWidth:0, background:'transparent', border:'none', color:PWC.txt, fontSize:11, padding:'4px 6px', outline:'none', fontFamily:PWmono }}/>
      {suffix && <span style={{ fontSize:9, color:PWC.txt3, padding:'0 6px', alignSelf:'center', borderLeft:`1px solid ${PWC.bd}` }}>{suffix}</span>}
      <div style={{ display:'flex', flexDirection:'column', borderLeft:`1px solid ${PWC.bd}` }}>
        {[['+', step],['−',-step]].map(([sym,d])=>(
          <button key={sym} onClick={()=>onChange(clamp(value+d))}
            style={{ flex:1, width:14, background:'transparent', border:'none', color:PWC.txt2, fontSize:9, cursor:'pointer', padding:0, lineHeight:1 }}
            onMouseEnter={e=>e.currentTarget.style.background=PWC.surf2}
            onMouseLeave={e=>e.currentTarget.style.background='transparent'}>
            {sym}
          </button>
        ))}
      </div>
    </div>
  );
}

function PWLabel({ children, hint }) {
  return (
    <label style={{ display:'block', fontSize:9, fontWeight:700, color:PWC.txt3, textTransform:'uppercase', letterSpacing:'0.08em', marginBottom:4 }}>
      {children}
      {hint && <span style={{ marginLeft:6, color:PWC.txt3, fontWeight:400, textTransform:'none', letterSpacing:'normal' }}>{hint}</span>}
    </label>
  );
}

function PWInput({ value, onChange, placeholder, error, autoFocus, type='text' }) {
  return (
    <input type={type} value={value} onChange={e=>onChange(e.target.value)}
      placeholder={placeholder} autoFocus={autoFocus}
      style={{
        width:'100%', background:PWC.bg, color:PWC.txt,
        border:`1px solid ${error?PWC.err:PWC.bd2}`,
        borderRadius:4, padding:'6px 9px', fontSize:12,
        fontFamily: type==='number' ? PWmono : PWsans, outline:'none',
      }}
      onFocus={e=>e.target.style.borderColor=error?PWC.err:PWC.acc}
      onBlur={e=>e.target.style.borderColor=error?PWC.err:PWC.bd2}/>
  );
}

/* ═════════════ mock camera / part SVG ═════════════ */
/* Reusable striped bg + a "part" silhouette so the user has something
   sensible to click on for a pick point and to place gripper boxes around. */
function PartImage({ width=CW, height=CH, brightness=1 }) {
  return (
    <svg width={width} height={height} viewBox="0 0 560 380" style={{ display:'block' }}>
      <defs>
        <pattern id="pwGrid" width="20" height="20" patternUnits="userSpaceOnUse">
          <rect width="20" height="20" fill="#2c2c2c"/>
          <path d="M20 0H0V20" fill="none" stroke="#363636" strokeWidth="0.6"/>
        </pattern>
        <linearGradient id="pwPart" x1="0" y1="0" x2="0" y2="1">
          <stop offset="0" stopColor="#7a7370"/>
          <stop offset="1" stopColor="#494442"/>
        </linearGradient>
        <linearGradient id="pwBolt" x1="0" y1="0" x2="0" y2="1">
          <stop offset="0" stopColor="#5b5552"/>
          <stop offset="1" stopColor="#3a3633"/>
        </linearGradient>
      </defs>
      <rect width="560" height="380" fill="url(#pwGrid)" opacity={brightness}/>
      {/* part body: rotated rect with rounded ends + central boss */}
      <g transform="translate(280 195) rotate(-14)" opacity={brightness}>
        <rect x="-160" y="-58" width="320" height="116" rx="22" fill="url(#pwPart)" stroke="#1e1a17" strokeWidth="2"/>
        <rect x="-150" y="-50" width="300" height="100" rx="18" fill="none" stroke="#94898355" strokeWidth="1"/>
        {/* central boss with hole — natural "pick here" */}
        <circle cx="0" cy="0" r="34" fill="url(#pwBolt)" stroke="#1e1a17" strokeWidth="1.5"/>
        <circle cx="0" cy="0" r="20" fill="#1c1816" stroke="#0d0b0a" strokeWidth="1"/>
        <circle cx="0" cy="0" r="12" fill="#0d0b0a"/>
        {/* mounting holes */}
        {[-1,1].map(s=>(
          <g key={s}>
            <circle cx={s*120} cy={-30} r="11" fill="#1c1816" stroke="#0d0b0a" strokeWidth="1"/>
            <circle cx={s*120} cy={ 30} r="11" fill="#1c1816" stroke="#0d0b0a" strokeWidth="1"/>
          </g>
        ))}
        {/* surface highlight */}
        <ellipse cx="-40" cy="-32" rx="80" ry="10" fill="#ffffff" opacity="0.06"/>
      </g>
      <rect x="0.5" y="0.5" width="559" height="379" fill="none" stroke="#0a0a0a" strokeWidth="1"/>
    </svg>
  );
}

/* ═════════════ STEP 1 — Image source ═════════════ */
function StepImage({ name, setName, num, setNum, nameErr, numErr, source, setSource, captured, setCaptured }) {
  return (
    <div style={{ display:'flex', gap:18, height:'100%' }}>
      {/* preview */}
      <div style={{ flex:1, background:'#181818', border:`1px solid ${PWC.bd}`, borderRadius:5, position:'relative', overflow:'hidden', display:'flex', alignItems:'center', justifyContent:'center' }}>
        {!captured ? (
          <div style={{ textAlign:'center', color:PWC.txt3 }}>
            <svg width="42" height="42" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.4" style={{ opacity:0.6 }}>
              <rect x="3" y="5" width="18" height="14" rx="2"/>
              <circle cx="12" cy="12" r="4"/>
              <path d="M9 5l1.5-2h3L15 5"/>
            </svg>
            <div style={{ fontSize:11, marginTop:10, fontFamily:PWmono }}>No image yet</div>
            <div style={{ fontSize:10, marginTop:3, color:PWC.txt3 }}>Capture from camera or open from file →</div>
          </div>
        ) : <PartImage/>}
        {captured && (
          <div style={{ position:'absolute', top:8, left:8, fontSize:9, color:PWC.ok, fontFamily:PWmono, background:'#0008', padding:'3px 7px', borderRadius:3, border:`1px solid ${PWC.ok}55` }}>
            ● {source==='camera' ? 'CAPTURED · 2448×2048' : 'LOADED · part_001.png'}
          </div>
        )}
        {captured && (
          <button onClick={()=>setCaptured(false)}
            style={{ position:'absolute', top:8, right:8, background:'#0009', border:`1px solid ${PWC.bd2}`, color:PWC.txt2, fontSize:10, padding:'3px 8px', borderRadius:3, cursor:'pointer', fontFamily:PWsans }}>
            ✕ Discard
          </button>
        )}
      </div>

      {/* right column */}
      <div style={{ width:280, display:'flex', flexDirection:'column', gap:14 }}>
        <div>
          <PWLabel>Pattern Name</PWLabel>
          <PWInput value={name} onChange={setName} placeholder="e.g. Front_face" error={!!nameErr} autoFocus/>
          {nameErr && <div style={{ fontSize:10, color:PWC.err, marginTop:4 }}>{nameErr}</div>}
        </div>
        <div>
          <PWLabel hint="written to output register on match">Pattern Number</PWLabel>
          <PWInput value={num} onChange={setNum} placeholder="1" error={!!numErr} type="number"/>
          {numErr && <div style={{ fontSize:10, color:PWC.err, marginTop:4 }}>{numErr}</div>}
        </div>

        <div style={{ height:1, background:PWC.bd, margin:'2px 0' }}/>

        <PWLabel>Image Source</PWLabel>
        <div style={{ display:'flex', flexDirection:'column', gap:8 }}>
          {[
            { id:'camera', label:'Capture from Camera', sub:'Trigger live camera and snapshot', icon:(
              <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.6" strokeLinecap="round">
                <rect x="3" y="6" width="18" height="13" rx="2"/><circle cx="12" cy="12.5" r="3.5"/>
                <path d="M9 6l1-2h4l1 2"/>
              </svg>) },
            { id:'file', label:'Open from File', sub:'Load PNG / JPG / BMP from disk', icon:(
              <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.6" strokeLinecap="round">
                <path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"/><path d="M14 2v6h6"/>
              </svg>) },
          ].map(opt=>{
            const sel = source===opt.id;
            return (
              <button key={opt.id}
                onClick={()=>{ setSource(opt.id); setCaptured(true); }}
                style={{
                  display:'flex', alignItems:'center', gap:10,
                  padding:'10px 12px', borderRadius:5, cursor:'pointer',
                  background: sel ? `${PWC.acc}22` : PWC.bg,
                  border:`1px solid ${sel?PWC.acc:PWC.bd2}`,
                  textAlign:'left', fontFamily:PWsans, color:PWC.txt,
                }}>
                <div style={{ color: sel ? PWC.acc : PWC.txt2, flexShrink:0 }}>{opt.icon}</div>
                <div style={{ flex:1, minWidth:0 }}>
                  <div style={{ fontSize:12, fontWeight:600 }}>{opt.label}</div>
                  <div style={{ fontSize:10, color:PWC.txt3, marginTop:1 }}>{opt.sub}</div>
                </div>
                {sel && <span style={{ fontSize:9, color:PWC.acc, fontFamily:PWmono, fontWeight:700 }}>● ACTIVE</span>}
              </button>
            );
          })}
        </div>
      </div>
    </div>
  );
}

/* ═════════════ STEP 2 — Crop ═════════════ */
function StepCrop({ keepOriginal, setKeepOriginal, crop, setCrop }) {
  const drag = useRef(null);
  const onCornerDrag = (corner) => (e) => {
    e.stopPropagation(); e.preventDefault();
    drag.current = { corner, startX:e.clientX, startY:e.clientY, c0:{...crop} };
    const onMove = ev => {
      const { corner, startX, startY, c0 } = drag.current;
      const dx = ev.clientX - startX, dy = ev.clientY - startY;
      let { x, y, w, h } = c0;
      if (corner.includes('l')) { x = c0.x + dx; w = c0.w - dx; }
      if (corner.includes('r')) { w = c0.w + dx; }
      if (corner.includes('t')) { y = c0.y + dy; h = c0.h - dy; }
      if (corner.includes('b')) { h = c0.h + dy; }
      x = Math.max(0, Math.min(CW-40, x));
      y = Math.max(0, Math.min(CH-40, y));
      w = Math.max(40, Math.min(CW-x, w));
      h = Math.max(40, Math.min(CH-y, h));
      setCrop({ x:Math.round(x), y:Math.round(y), w:Math.round(w), h:Math.round(h) });
    };
    const onUp = () => { window.removeEventListener('mousemove', onMove); window.removeEventListener('mouseup', onUp); };
    window.addEventListener('mousemove', onMove);
    window.addEventListener('mouseup', onUp);
  };

  return (
    <div style={{ display:'flex', gap:18, height:'100%' }}>
      <div style={{ flex:1, background:'#181818', border:`1px solid ${PWC.bd}`, borderRadius:5, position:'relative', overflow:'hidden' }}>
        <PartImage brightness={keepOriginal ? 1 : 0.45}/>
        {!keepOriginal && (
          <>
            {/* darken outside crop using 4 strips */}
            <div style={{ position:'absolute', left:0, right:0, top:0, height:crop.y, background:'#000c' }}/>
            <div style={{ position:'absolute', left:0, right:0, top:crop.y+crop.h, bottom:0, background:'#000c' }}/>
            <div style={{ position:'absolute', top:crop.y, left:0, width:crop.x, height:crop.h, background:'#000c' }}/>
            <div style={{ position:'absolute', top:crop.y, left:crop.x+crop.w, right:0, height:crop.h, background:'#000c' }}/>
            {/* crop rect */}
            <div style={{ position:'absolute', left:crop.x, top:crop.y, width:crop.w, height:crop.h, border:`1.5px solid ${PWC.acc}`, boxSizing:'border-box' }}>
              {/* rule-of-thirds */}
              <div style={{ position:'absolute', left:'33.33%', top:0, bottom:0, width:1, background:`${PWC.acc}55` }}/>
              <div style={{ position:'absolute', left:'66.66%', top:0, bottom:0, width:1, background:`${PWC.acc}55` }}/>
              <div style={{ position:'absolute', top:'33.33%', left:0, right:0, height:1, background:`${PWC.acc}55` }}/>
              <div style={{ position:'absolute', top:'66.66%', left:0, right:0, height:1, background:`${PWC.acc}55` }}/>
              {/* corners */}
              {['tl','tr','bl','br'].map(k=>{
                const [v,h] = [k[0]==='t'?'top':'bottom', k[1]==='l'?'left':'right'];
                return <div key={k} onMouseDown={onCornerDrag(k)} style={{
                  position:'absolute', [v]:-5, [h]:-5, width:11, height:11,
                  background:PWC.acc, border:'1.5px solid #fff', cursor:`${v[0]}${h[0]}-resize`,
                }}/>;
              })}
            </div>
          </>
        )}
        <div style={{ position:'absolute', bottom:8, left:10, fontSize:9, color:PWC.txt3, fontFamily:PWmono }}>
          Source 2448 × 2048 · {keepOriginal ? 'keeping full frame' : `crop ${Math.round(crop.w*4.37)} × ${Math.round(crop.h*5.39)} px`}
        </div>
      </div>

      <div style={{ width:280, display:'flex', flexDirection:'column', gap:14 }}>
        <label style={{
          display:'flex', alignItems:'center', gap:10, padding:'10px 12px',
          background:keepOriginal?`${PWC.acc}22`:PWC.bg, border:`1px solid ${keepOriginal?PWC.acc:PWC.bd2}`,
          borderRadius:5, cursor:'pointer',
        }}>
          <input type="checkbox" checked={keepOriginal} onChange={e=>setKeepOriginal(e.target.checked)} style={{ accentColor:PWC.acc }}/>
          <div>
            <div style={{ fontSize:12, fontWeight:600, color:PWC.txt }}>Use original frame</div>
            <div style={{ fontSize:10, color:PWC.txt3, marginTop:1 }}>Skip cropping; use the full image as the pattern.</div>
          </div>
        </label>

        <div style={{ opacity:keepOriginal?0.4:1, pointerEvents:keepOriginal?'none':'auto', display:'flex', flexDirection:'column', gap:12 }}>
          <PWLabel>Crop Region (display px)</PWLabel>
          <div style={{ display:'grid', gridTemplateColumns:'auto 1fr', gap:'8px 10px', alignItems:'center' }}>
            <span style={{ fontSize:10, color:PWC.txt3, fontFamily:PWmono }}>X</span>
            <PWNum value={crop.x} onChange={v=>setCrop({...crop, x:v})} min={0} max={CW-crop.w} suffix="px" w="100%"/>
            <span style={{ fontSize:10, color:PWC.txt3, fontFamily:PWmono }}>Y</span>
            <PWNum value={crop.y} onChange={v=>setCrop({...crop, y:v})} min={0} max={CH-crop.h} suffix="px" w="100%"/>
            <span style={{ fontSize:10, color:PWC.txt3, fontFamily:PWmono }}>W</span>
            <PWNum value={crop.w} onChange={v=>setCrop({...crop, w:v})} min={40} max={CW-crop.x} suffix="px" w="100%"/>
            <span style={{ fontSize:10, color:PWC.txt3, fontFamily:PWmono }}>H</span>
            <PWNum value={crop.h} onChange={v=>setCrop({...crop, h:v})} min={40} max={CH-crop.y} suffix="px" w="100%"/>
          </div>
          <div style={{ display:'flex', gap:6 }}>
            <PWBtn onClick={()=>setCrop({ x:80, y:60, w:CW-160, h:CH-120 })}>Reset</PWBtn>
            <PWBtn onClick={()=>{
              const s=Math.min(CW-40, CH-40);
              setCrop({ x:Math.round((CW-s)/2), y:Math.round((CH-s)/2), w:s, h:s });
            }}>1:1 Center</PWBtn>
          </div>
        </div>
      </div>
    </div>
  );
}

/* ═════════════ STEP 3 — Pick point ═════════════ */
function StepPick({ pick, setPick }) {
  const onClick = (e) => {
    const r = e.currentTarget.getBoundingClientRect();
    setPick({
      x: Math.round(e.clientX - r.left),
      y: Math.round(e.clientY - r.top),
    });
  };
  return (
    <div style={{ display:'flex', gap:18, height:'100%' }}>
      <div onClick={onClick}
        style={{ flex:1, background:'#181818', border:`1px solid ${PWC.bd}`, borderRadius:5, position:'relative', overflow:'hidden', cursor:'crosshair' }}>
        <PartImage/>
        {/* crosshair */}
        <div style={{ position:'absolute', left:0, right:0, top:pick.y, height:1, background:`${PWC.warn}cc`, pointerEvents:'none' }}/>
        <div style={{ position:'absolute', top:0, bottom:0, left:pick.x, width:1, background:`${PWC.warn}cc`, pointerEvents:'none' }}/>
        <div style={{ position:'absolute', left:pick.x-9, top:pick.y-9, width:18, height:18, border:`1.5px solid ${PWC.warn}`, borderRadius:'50%', pointerEvents:'none', background:`${PWC.warn}22` }}/>
        <div style={{ position:'absolute', left:pick.x+12, top:pick.y-22, fontSize:9, fontFamily:PWmono, fontWeight:700, color:PWC.warn, background:'#000a', padding:'2px 5px', borderRadius:2, pointerEvents:'none' }}>
          PICK ({pick.x}, {pick.y})
        </div>
        <div style={{ position:'absolute', bottom:8, left:10, fontSize:9, color:PWC.txt3, fontFamily:PWmono }}>
          Click anywhere on the pattern to set the picking position
        </div>
      </div>

      <div style={{ width:280, display:'flex', flexDirection:'column', gap:14 }}>
        <PWLabel>Picking Position (image coords)</PWLabel>
        <div style={{ display:'grid', gridTemplateColumns:'auto 1fr', gap:'10px 10px', alignItems:'center' }}>
          <span style={{ fontSize:11, color:PWC.acc, fontFamily:PWmono, fontWeight:700 }}>X</span>
          <PWNum value={pick.x} onChange={v=>setPick({...pick, x:v})} min={0} max={CW} suffix="px" w="100%"/>
          <span style={{ fontSize:11, color:PWC.acc, fontFamily:PWmono, fontWeight:700 }}>Y</span>
          <PWNum value={pick.y} onChange={v=>setPick({...pick, y:v})} min={0} max={CH} suffix="px" w="100%"/>
        </div>
        <div style={{ display:'flex', gap:6 }}>
          <PWBtn onClick={()=>setPick({ x:Math.round(CW/2), y:Math.round(CH/2) })}>Center</PWBtn>
          <PWBtn onClick={()=>setPick({ x:280, y:195 })}>Snap to Boss</PWBtn>
        </div>

        <div style={{ height:1, background:PWC.bd, margin:'4px 0' }}/>
        <div style={{ background:PWC.bg, border:`1px solid ${PWC.bd}`, borderRadius:5, padding:'10px 12px' }}>
          <div style={{ fontSize:10, fontWeight:700, color:PWC.txt3, textTransform:'uppercase', letterSpacing:'0.08em', marginBottom:6 }}>How it's used</div>
          <div style={{ fontSize:11, color:PWC.txt2, lineHeight:1.45 }}>
            On a match, this pattern-relative offset is transformed by the detected pose to produce the real-world TCP target sent to the robot.
          </div>
        </div>
      </div>
    </div>
  );
}

/* ═════════════ STEP 4 — Picking box ═════════════ */
function rotated(cx, cy, x, y, deg) {
  const r = deg * Math.PI / 180, c = Math.cos(r), s = Math.sin(r);
  return [cx + (x-cx)*c - (y-cy)*s, cy + (x-cx)*s + (y-cy)*c];
}
function BoxOverlay({ pick, box, color, label }) {
  // box center: pick + (cos(angle)*dist, sin(angle)*dist)
  const r = box.angle * Math.PI / 180;
  const cx = pick.x + Math.cos(r) * box.dist;
  const cy = pick.y + Math.sin(r) * box.dist;
  // 4 corners of axis-aligned rect, rotated by angle around (cx,cy)
  const w = box.w/2, h = box.h/2;
  const pts = [[-w,-h],[w,-h],[w,h],[-w,h]].map(([x,y])=>{
    const cr = Math.cos(r), sr = Math.sin(r);
    return [cx + x*cr - y*sr, cy + x*sr + y*cr];
  });
  const path = pts.map((p,i)=> (i===0?'M':'L') + p[0].toFixed(1)+','+p[1].toFixed(1)).join(' ') + 'Z';
  return (
    <g>
      {/* connector from pick to box center */}
      <line x1={pick.x} y1={pick.y} x2={cx} y2={cy} stroke={color} strokeWidth="1" strokeDasharray="3 3" opacity="0.55"/>
      <path d={path} fill={`${color}22`} stroke={color} strokeWidth="1.5"/>
      <circle cx={cx} cy={cy} r="2.5" fill={color}/>
      <text x={cx} y={cy-h-4} fill={color} fontSize="9" fontFamily={PWmono} fontWeight="700" textAnchor="middle">{label}</text>
    </g>
  );
}
function StepBox({ pick, boxCfg, setBoxCfg }) {
  // Symmetric pair: both jaws share width/height/distance; angles are opposite.
  const boxA = { w:boxCfg.w, h:boxCfg.h, dist:boxCfg.dist, angle:boxCfg.angle };
  const boxB = { w:boxCfg.w, h:boxCfg.h, dist:boxCfg.dist, angle:boxCfg.angle+180 };
  return (
    <div style={{ display:'flex', gap:18, height:'100%' }}>
      <div style={{ flex:1, background:'#181818', border:`1px solid ${PWC.bd}`, borderRadius:5, position:'relative', overflow:'hidden' }}>
        <PartImage/>
        <svg width={CW} height={CH} style={{ position:'absolute', inset:0, pointerEvents:'none' }}>
          <BoxOverlay pick={pick} box={boxA} color="#4ec9b0" label="A"/>
          <BoxOverlay pick={pick} box={boxB} color="#4ec9b0" label="B"/>
          {/* pick crosshair */}
          <line x1={pick.x-12} y1={pick.y} x2={pick.x+12} y2={pick.y} stroke="#fff" strokeWidth="1"/>
          <line x1={pick.x} y1={pick.y-12} x2={pick.x} y2={pick.y+12} stroke="#fff" strokeWidth="1"/>
          <circle cx={pick.x} cy={pick.y} r="3" fill="#fff"/>
        </svg>
        <div style={{ position:'absolute', bottom:8, left:10, fontSize:9, color:PWC.txt3, fontFamily:PWmono }}>
          Symmetric gripper jaws — same size, mirrored about the pick point
        </div>
      </div>

      <div style={{ width:280, display:'flex', flexDirection:'column', gap:12 }}>
        <div style={{ background:`${PWC.acc}1a`, border:`1px solid ${PWC.acc}55`, borderRadius:5, padding:'8px 11px', fontSize:10, color:PWC.txt2, lineHeight:1.45 }}>
          Both jaws share the same dimensions and offset. Box A sits at <strong style={{ color:PWC.acc, fontFamily:PWmono }}>angle</strong>, Box B at <strong style={{ color:PWC.acc, fontFamily:PWmono }}>angle + 180°</strong>.
        </div>

        <PWLabel>Box Size (shared)</PWLabel>
        <div style={{ display:'grid', gridTemplateColumns:'auto 1fr', gap:'8px 10px', alignItems:'center' }}>
          <span style={{ fontSize:10, color:PWC.txt3, fontFamily:PWmono }}>Width</span>
          <PWNum value={boxCfg.w} onChange={v=>setBoxCfg({...boxCfg, w:v})} min={5} max={CW} suffix="px" w="100%"/>
          <span style={{ fontSize:10, color:PWC.txt3, fontFamily:PWmono }}>Height</span>
          <PWNum value={boxCfg.h} onChange={v=>setBoxCfg({...boxCfg, h:v})} min={5} max={CH} suffix="px" w="100%"/>
        </div>

        <PWLabel>Offset from Pick Point</PWLabel>
        <div style={{ display:'grid', gridTemplateColumns:'auto 1fr', gap:'8px 10px', alignItems:'center' }}>
          <span style={{ fontSize:10, color:PWC.txt3, fontFamily:PWmono }}>Distance</span>
          <PWNum value={boxCfg.dist} onChange={v=>setBoxCfg({...boxCfg, dist:v})} min={0} max={400} suffix="px" w="100%"/>
          <span style={{ fontSize:10, color:PWC.txt3, fontFamily:PWmono }}>Angle</span>
          <PWNum value={boxCfg.angle} onChange={v=>setBoxCfg({...boxCfg, angle:v})} min={-180} max={180} suffix="°" w="100%"/>
        </div>

        <div style={{ display:'flex', gap:6 }}>
          <PWBtn onClick={()=>setBoxCfg({ w:120, h:80, dist:90, angle:0 })}>Reset</PWBtn>
          <PWBtn onClick={()=>setBoxCfg({...boxCfg, angle:boxCfg.angle+90})}>Rotate +90°</PWBtn>
        </div>

        <div style={{ height:1, background:PWC.bd, margin:'2px 0' }}/>
        <div style={{ fontSize:10, color:PWC.txt3, lineHeight:1.5 }}>
          The two boxes represent the gripper jaws on the actual hardware. If a neighbouring object falls inside either box, the candidate is rejected to avoid collisions.
        </div>
      </div>
    </div>
  );
}

/* ═════════════ STEP 5 — Finish / Review ═════════════ */
function StepFinish({ name, num, source, keepOriginal, crop, pick, boxCfg }) {
  const boxA = { w:boxCfg.w, h:boxCfg.h, dist:boxCfg.dist, angle:boxCfg.angle };
  const boxB = { w:boxCfg.w, h:boxCfg.h, dist:boxCfg.dist, angle:boxCfg.angle+180 };
  const Row = ({ k, v }) => (
    <div style={{ display:'flex', justifyContent:'space-between', padding:'5px 0', borderBottom:`1px dashed ${PWC.bd}` }}>
      <span style={{ fontSize:10, color:PWC.txt3, fontWeight:700, textTransform:'uppercase', letterSpacing:'0.06em' }}>{k}</span>
      <span style={{ fontSize:11, color:PWC.txt, fontFamily:PWmono }}>{v}</span>
    </div>
  );
  return (
    <div style={{ display:'flex', gap:18, height:'100%' }}>
      <div style={{ flex:1, background:'#181818', border:`1px solid ${PWC.bd}`, borderRadius:5, position:'relative', overflow:'hidden' }}>
        <PartImage/>
        <svg width={CW} height={CH} style={{ position:'absolute', inset:0, pointerEvents:'none' }}>
          <BoxOverlay pick={pick} box={boxA} color="#4ec9b0" label="A"/>
          <BoxOverlay pick={pick} box={boxB} color="#4ec9b0" label="B"/>
          <line x1={pick.x-12} y1={pick.y} x2={pick.x+12} y2={pick.y} stroke="#fff" strokeWidth="1"/>
          <line x1={pick.x} y1={pick.y-12} x2={pick.x} y2={pick.y+12} stroke="#fff" strokeWidth="1"/>
          <circle cx={pick.x} cy={pick.y} r="3" fill={PWC.warn}/>
        </svg>
        <div style={{ position:'absolute', top:8, left:8, fontSize:9, fontFamily:PWmono, fontWeight:700, color:PWC.ok, background:'#000a', padding:'3px 8px', borderRadius:3, border:`1px solid ${PWC.ok}55` }}>
          ● PATTERN READY
        </div>
      </div>

      <div style={{ width:280, display:'flex', flexDirection:'column', gap:6 }}>
        <div style={{ fontSize:13, fontWeight:700, color:PWC.txt, marginBottom:4 }}>{name || '(unnamed)'} <span style={{ color:PWC.acc, fontFamily:PWmono, marginLeft:4 }}>#{num||'—'}</span></div>
        <Row k="Source" v={source==='camera'?'Camera capture':'File import'}/>
        <Row k="Crop" v={keepOriginal?'Original (no crop)':`${crop.w}×${crop.h} @ (${crop.x},${crop.y})`}/>
        <Row k="Pick Point" v={`(${pick.x}, ${pick.y}) px`}/>
        <Row k="Box Size" v={`${boxCfg.w} × ${boxCfg.h} px`}/>
        <Row k="Offset" v={`d=${boxCfg.dist} · ${boxCfg.angle}° / ${boxCfg.angle+180}°`}/>
        <div style={{ marginTop:10, background:`${PWC.ok}15`, border:`1px solid ${PWC.ok}55`, borderRadius:5, padding:'9px 11px', fontSize:11, color:PWC.txt2, lineHeight:1.45 }}>
          On <strong style={{ color:PWC.ok }}>Apply</strong>, the pattern is added to the group and marked as learned. You can refine thresholds later in the Property panel.
        </div>
      </div>
    </div>
  );
}

/* ═════════════ MAIN WIZARD ═════════════ */
function PatternWizard({ groupName, usedNums, usedNames, onConfirm, onClose }) {
  const STEPS = [
    { id:'image',  label:'Image',        sub:'Capture or load source' },
    { id:'crop',   label:'Crop',         sub:'Trim to pattern region' },
    { id:'pick',   label:'Pick Point',   sub:'Set picking position' },
    { id:'box',    label:'Picking Box',  sub:'Define gripper bounds' },
    { id:'done',   label:'Finish',       sub:'Review & apply' },
  ];
  const [step, setStep] = useState(0);

  // step 1
  const [name, setName] = useState('');
  const [num,  setNum]  = useState('');
  const [source, setSource] = useState(null);
  const [captured, setCaptured] = useState(false);

  // step 2
  const [keepOriginal, setKeepOriginal] = useState(true);
  const [crop, setCrop] = useState({ x:80, y:60, w:CW-160, h:CH-120 });

  // step 3
  const [pick, setPick] = useState({ x:280, y:195 });

  // step 4 — symmetric pair, single shared size + offset
  const [boxCfg, setBoxCfg] = useState({ w:120, h:80, dist:90, angle:0 });

  // validation
  const trimmed = name.trim();
  const n = parseInt(num) || 0;
  const nameErr = trimmed && usedNames.includes(trimmed) ? 'Name already exists in group' : null;
  const numErr  = num && (usedNums.includes(n) || n<1) ? (n<1 ? 'Must be ≥ 1' : 'Number already exists') : null;
  const step1Ok = trimmed && num && !nameErr && !numErr && captured;

  const canNext =
    step===0 ? !!step1Ok :
    step===1 ? true :
    step===2 ? true :
    step===3 ? true : true;

  const finish = () => onConfirm({
    name: trimmed, number: n,
    image: { source, width: CW, height: CH },
    crop:  keepOriginal ? null : crop,
    pickPos: pick,
    pickingBox: { w:boxCfg.w, h:boxCfg.h, dist:boxCfg.dist, angle:boxCfg.angle },
    boxA: { w:boxCfg.w, h:boxCfg.h, dist:boxCfg.dist, angle:boxCfg.angle },
    boxB: { w:boxCfg.w, h:boxCfg.h, dist:boxCfg.dist, angle:boxCfg.angle+180 },
  });

  return (
    <div onClick={onClose}
      style={{ position:'fixed', inset:0, background:'rgba(5,9,18,0.78)', display:'flex', alignItems:'center', justifyContent:'center', zIndex:2000, backdropFilter:'blur(3px)' }}>
      <div onClick={e=>e.stopPropagation()}
        style={{ width:920, height:640, background:PWC.surf2, border:`1px solid ${PWC.bd2}`, borderRadius:9, overflow:'hidden', boxShadow:'0 24px 64px rgba(0,0,0,0.6)', display:'flex', flexDirection:'column' }}>
        {/* header */}
        <div style={{ padding:'12px 18px', background:PWC.hd, borderBottom:`1px solid ${PWC.bd}`, display:'flex', alignItems:'center', justifyContent:'space-between' }}>
          <div>
            <div style={{ fontSize:13, fontWeight:700, color:PWC.txt }}>New Pattern Wizard</div>
            <div style={{ fontSize:10, color:PWC.txt3, marginTop:2 }}>Group: {groupName}  ·  step {step+1} of {STEPS.length} — {STEPS[step].sub}</div>
          </div>
          <button onClick={onClose} style={{ background:'transparent', border:'none', color:PWC.txt2, fontSize:18, cursor:'pointer', padding:4, lineHeight:1 }}>✕</button>
        </div>

        {/* step rail */}
        <div style={{ display:'flex', alignItems:'stretch', background:PWC.bg, borderBottom:`1px solid ${PWC.bd}`, padding:'0 18px' }}>
          {STEPS.map((s, i) => {
            const done = i < step;
            const cur  = i === step;
            const reachable = i <= step || (i===step+1 && canNext);
            return (
              <button key={s.id} onClick={()=>reachable && setStep(i)}
                style={{
                  flex:1, padding:'10px 8px', display:'flex', alignItems:'center', gap:8,
                  background:'transparent', border:'none', cursor:reachable?'pointer':'not-allowed',
                  borderBottom:`2px solid ${cur ? PWC.acc : 'transparent'}`,
                  fontFamily:PWsans, textAlign:'left',
                }}>
                <div style={{
                  width:22, height:22, borderRadius:'50%',
                  display:'flex', alignItems:'center', justifyContent:'center',
                  fontSize:10, fontWeight:700, fontFamily:PWmono,
                  background: done ? PWC.ok : cur ? PWC.acc : PWC.surf,
                  color: (done||cur) ? '#fff' : PWC.txt3,
                  border: `1px solid ${done?PWC.ok:cur?PWC.acc:PWC.bd2}`,
                }}>
                  {done ? '✓' : i+1}
                </div>
                <div style={{ minWidth:0 }}>
                  <div style={{ fontSize:11, fontWeight:600, color: cur ? PWC.txt : done ? PWC.txt2 : PWC.txt3 }}>{s.label}</div>
                  <div style={{ fontSize:9, color:PWC.txt3, marginTop:1, whiteSpace:'nowrap', overflow:'hidden', textOverflow:'ellipsis' }}>{s.sub}</div>
                </div>
              </button>
            );
          })}
        </div>

        {/* body */}
        <div style={{ flex:1, padding:18, overflow:'hidden' }}>
          {step===0 && <StepImage {...{ name, setName, num, setNum, nameErr, numErr, source, setSource, captured, setCaptured }}/>}
          {step===1 && <StepCrop  {...{ keepOriginal, setKeepOriginal, crop, setCrop }}/>}
          {step===2 && <StepPick  {...{ pick, setPick }}/>}
          {step===3 && <StepBox   {...{ pick, boxCfg, setBoxCfg }}/>}
          {step===4 && <StepFinish {...{ name:trimmed, num:n, source, keepOriginal, crop, pick, boxCfg }}/>}
        </div>

        {/* footer */}
        <div style={{ padding:'12px 18px', borderTop:`1px solid ${PWC.bd}`, background:PWC.hd, display:'flex', alignItems:'center', justifyContent:'space-between' }}>
          <div style={{ fontSize:10, color:PWC.txt3, fontFamily:PWmono }}>
            {step===0 && !step1Ok && (
              !trimmed || !num ? 'Enter a name and number to continue.'
              : !captured ? 'Capture or load an image to continue.'
              : 'Resolve the highlighted errors.')}
            {step===0 && step1Ok && '✓ Image ready · proceed to crop'}
            {step===1 && (keepOriginal ? '✓ Using original frame' : `✓ Cropped to ${crop.w}×${crop.h}px`)}
            {step===2 && `✓ Pick point at (${pick.x}, ${pick.y})`}
            {step===3 && `✓ Symmetric pair · ${boxCfg.w}×${boxCfg.h} · d=${boxCfg.dist} · ±${boxCfg.angle}°`}
            {step===4 && '✓ All steps complete — ready to apply'}
          </div>
          <div style={{ display:'flex', gap:8 }}>
            <PWBtn onClick={onClose}>Cancel</PWBtn>
            <PWBtn onClick={()=>setStep(s=>Math.max(0, s-1))} disabled={step===0}>← Back</PWBtn>
            {step < STEPS.length-1 ? (
              <PWBtn primary onClick={()=>canNext && setStep(s=>s+1)} disabled={!canNext}>Next →</PWBtn>
            ) : (
              <PWBtn primary onClick={finish}>✓ Apply Pattern</PWBtn>
            )}
          </div>
        </div>
      </div>
    </div>
  );
}

window.PatternWizard = PatternWizard;
