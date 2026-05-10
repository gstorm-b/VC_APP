/* ───────────────────────────── EditPatternWizard.jsx
   4-step wizard: Identity → Pick Point → Picking Box → Finish
   Image is already saved → locked (read-only preview only).
   Pre-populated from the existing pattern data.
   Exports window.EditPatternWizard for use by PatternManager.jsx.
   ────────────────────────────────────────────────────────── */
const { useState, useRef, useEffect } = React;

/* ── Palette / fonts (mirror PatternWizard) ── */
const EWC = {
  bg:'#1e1e1e', surf:'#252526', surf2:'#2d2d2d',
  bd:'#3c3c3c', bd2:'#454545',
  txt:'#cccccc', txt2:'#9d9d9d', txt3:'#6a6a6a',
  acc:'#0e639c', accHi:'#1177bb',
  ok:'#4ec9b0', warn:'#dcb67a', err:'#f48771',
  hd:'#1e1e1e', lock:'#6a5acd',
};
const EWmono = 'JetBrains Mono, monospace';
const EWsans = 'Space Grotesk, sans-serif';

/* canvas display dims */
const CW = 560, CH = 380;

/* ══════════ shared atoms ══════════ */
function EWBtn({ children, onClick, active, primary, disabled, title }) {
  const [h, setH] = useState(false);
  const bg = disabled ? EWC.surf
    : primary ? (h ? EWC.accHi : EWC.acc)
    : active  ? `${EWC.acc}33`
    : h ? EWC.surf2 : 'transparent';
  const color = disabled ? EWC.txt3
    : primary ? '#fff' : active ? EWC.acc : EWC.txt;
  const bd = primary ? 'transparent'
    : active ? EWC.acc
    : h ? EWC.bd2 : EWC.bd;
  return (
    <button title={title} disabled={disabled}
      onMouseEnter={() => setH(true)} onMouseLeave={() => setH(false)}
      onClick={onClick}
      style={{
        background: bg, color, border: `1px solid ${bd}`,
        padding: primary ? '7px 18px' : '5px 12px',
        fontSize: 11, fontWeight: primary ? 700 : 500,
        fontFamily: EWsans, borderRadius: 4,
        cursor: disabled ? 'not-allowed' : 'pointer',
        display: 'inline-flex', alignItems: 'center', gap: 6,
        transition: 'all 0.12s', whiteSpace: 'nowrap',
      }}>
      {children}
    </button>
  );
}

function EWNum({ value, onChange, min, max, step = 1, suffix, w = 82 }) {
  const clamp = v => {
    if (Number.isNaN(v)) return min ?? 0;
    if (min !== undefined && v < min) return min;
    if (max !== undefined && v > max) return max;
    return v;
  };
  return (
    <div style={{ display: 'flex', alignItems: 'stretch', width: w, border: `1px solid ${EWC.bd2}`, borderRadius: 4, background: EWC.bg, fontFamily: EWmono }}>
      <input type="number" value={value}
        onChange={e => onChange(clamp(parseFloat(e.target.value)))}
        step={step} min={min} max={max}
        style={{ flex: 1, minWidth: 0, background: 'transparent', border: 'none', color: EWC.txt, fontSize: 11, padding: '4px 6px', outline: 'none', fontFamily: EWmono }} />
      {suffix && <span style={{ fontSize: 9, color: EWC.txt3, padding: '0 6px', alignSelf: 'center', borderLeft: `1px solid ${EWC.bd}` }}>{suffix}</span>}
      <div style={{ display: 'flex', flexDirection: 'column', borderLeft: `1px solid ${EWC.bd}` }}>
        {[['+', step], ['−', -step]].map(([sym, d]) => (
          <button key={sym} onClick={() => onChange(clamp(value + d))}
            style={{ flex: 1, width: 14, background: 'transparent', border: 'none', color: EWC.txt2, fontSize: 9, cursor: 'pointer', padding: 0, lineHeight: 1 }}
            onMouseEnter={e => e.currentTarget.style.background = EWC.surf2}
            onMouseLeave={e => e.currentTarget.style.background = 'transparent'}>
            {sym}
          </button>
        ))}
      </div>
    </div>
  );
}

function EWLabel({ children, hint }) {
  return (
    <label style={{ display: 'block', fontSize: 9, fontWeight: 700, color: EWC.txt3, textTransform: 'uppercase', letterSpacing: '0.08em', marginBottom: 4 }}>
      {children}
      {hint && <span style={{ marginLeft: 6, color: EWC.txt3, fontWeight: 400, textTransform: 'none', letterSpacing: 'normal' }}>{hint}</span>}
    </label>
  );
}

function EWInput({ value, onChange, placeholder, error, autoFocus, type = 'text' }) {
  return (
    <input type={type} value={value} onChange={e => onChange(e.target.value)}
      placeholder={placeholder} autoFocus={autoFocus}
      style={{
        width: '100%', background: EWC.bg, color: EWC.txt,
        border: `1px solid ${error ? EWC.err : EWC.bd2}`,
        borderRadius: 4, padding: '6px 9px', fontSize: 12,
        fontFamily: type === 'number' ? EWmono : EWsans, outline: 'none',
      }}
      onFocus={e => e.target.style.borderColor = error ? EWC.err : EWC.acc}
      onBlur={e => e.target.style.borderColor = error ? EWC.err : EWC.bd2} />
  );
}

/* ══════════ mock part image (same as PatternWizard) ══════════ */
function PartImage({ width = CW, height = CH, dim = false }) {
  return (
    <svg width={width} height={height} viewBox="0 0 560 380" style={{ display: 'block', opacity: dim ? 0.4 : 1 }}>
      <defs>
        <pattern id="ewGrid" width="20" height="20" patternUnits="userSpaceOnUse">
          <rect width="20" height="20" fill="#2c2c2c" />
          <path d="M20 0H0V20" fill="none" stroke="#363636" strokeWidth="0.6" />
        </pattern>
        <linearGradient id="ewPart" x1="0" y1="0" x2="0" y2="1">
          <stop offset="0" stopColor="#7a7370" />
          <stop offset="1" stopColor="#494442" />
        </linearGradient>
        <linearGradient id="ewBolt" x1="0" y1="0" x2="0" y2="1">
          <stop offset="0" stopColor="#5b5552" />
          <stop offset="1" stopColor="#3a3633" />
        </linearGradient>
      </defs>
      <rect width="560" height="380" fill="url(#ewGrid)" />
      <g transform="translate(280 195) rotate(-14)">
        <rect x="-160" y="-58" width="320" height="116" rx="22" fill="url(#ewPart)" stroke="#1e1a17" strokeWidth="2" />
        <rect x="-150" y="-50" width="300" height="100" rx="18" fill="none" stroke="#94898355" strokeWidth="1" />
        <circle cx="0" cy="0" r="34" fill="url(#ewBolt)" stroke="#1e1a17" strokeWidth="1.5" />
        <circle cx="0" cy="0" r="20" fill="#1c1816" stroke="#0d0b0a" strokeWidth="1" />
        <circle cx="0" cy="0" r="12" fill="#0d0b0a" />
        {[-1, 1].map(s => (
          <g key={s}>
            <circle cx={s * 120} cy={-30} r="11" fill="#1c1816" stroke="#0d0b0a" strokeWidth="1" />
            <circle cx={s * 120} cy={30} r="11" fill="#1c1816" stroke="#0d0b0a" strokeWidth="1" />
          </g>
        ))}
        <ellipse cx="-40" cy="-32" rx="80" ry="10" fill="#ffffff" opacity="0.06" />
      </g>
      <rect x="0.5" y="0.5" width="559" height="379" fill="none" stroke="#0a0a0a" strokeWidth="1" />
    </svg>
  );
}

/* ══════════ STEP 1 — Identity ══════════
   Left: locked image preview  |  Right: editable name + number            */
function StepIdentity({ origName, origNum, name, setName, num, setNum, nameErr, numErr }) {
  return (
    <div style={{ display: 'flex', gap: 18, height: '100%' }}>

      {/* — locked image preview — */}
      <div style={{ flex: 1, background: '#181818', border: `1px solid ${EWC.bd}`, borderRadius: 5, position: 'relative', overflow: 'hidden', display: 'flex', alignItems: 'center', justifyContent: 'center' }}>
        <PartImage />

        {/* lock scrim */}
        <div style={{ position: 'absolute', inset: 0, background: 'rgba(0,0,0,0.38)', pointerEvents: 'none' }} />

        {/* lock badge */}
        <div style={{
          position: 'absolute', top: 10, left: 10,
          display: 'flex', alignItems: 'center', gap: 6,
          background: `${EWC.lock}cc`, border: `1px solid ${EWC.lock}`,
          borderRadius: 4, padding: '4px 9px',
        }}>
          <svg width="11" height="11" viewBox="0 0 24 24" fill="none" stroke="#fff" strokeWidth="2.2" strokeLinecap="round">
            <rect x="3" y="11" width="18" height="11" rx="2" />
            <path d="M7 11V7a5 5 0 0 1 10 0v4" />
          </svg>
          <span style={{ fontSize: 9, fontWeight: 700, color: '#fff', fontFamily: EWmono, letterSpacing: '0.06em' }}>IMAGE LOCKED</span>
        </div>

        {/* saved tag */}
        <div style={{ position: 'absolute', bottom: 8, left: 10, fontSize: 9, color: EWC.ok, fontFamily: EWmono, background: '#000a', padding: '3px 7px', borderRadius: 3, border: `1px solid ${EWC.ok}55` }}>
          ● SAVED · 2448 × 2048
        </div>

        {/* can't change note */}
        <div style={{
          position: 'absolute', bottom: 8, right: 10,
          fontSize: 9, color: EWC.txt3, fontFamily: EWmono, background: '#000a',
          padding: '3px 7px', borderRadius: 3,
        }}>
          To replace image, delete & re-add pattern
        </div>
      </div>

      {/* — right column — */}
      <div style={{ width: 280, display: 'flex', flexDirection: 'column', gap: 16 }}>

        {/* info notice */}
        <div style={{ background: `${EWC.lock}18`, border: `1px solid ${EWC.lock}44`, borderRadius: 5, padding: '9px 12px', display: 'flex', gap: 8, alignItems: 'flex-start' }}>
          <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke={EWC.lock} strokeWidth="2" strokeLinecap="round" style={{ flexShrink: 0, marginTop: 1 }}>
            <rect x="3" y="11" width="18" height="11" rx="2" />
            <path d="M7 11V7a5 5 0 0 1 10 0v4" />
          </svg>
          <div style={{ fontSize: 10, color: EWC.txt2, lineHeight: 1.5 }}>
            The pattern image is <strong style={{ color: EWC.txt }}>locked</strong> after learning. You can update the name, number, picking position, and gripper box without retraining.
          </div>
        </div>

        {/* name */}
        <div>
          <EWLabel>Pattern Name</EWLabel>
          <EWInput value={name} onChange={setName} placeholder="e.g. Front_face" error={!!nameErr} autoFocus />
          {nameErr
            ? <div style={{ fontSize: 10, color: EWC.err, marginTop: 4 }}>{nameErr}</div>
            : name !== origName && <div style={{ fontSize: 10, color: EWC.warn, marginTop: 4, fontFamily: EWmono }}>← was: {origName}</div>}
        </div>

        {/* number */}
        <div>
          <EWLabel hint="output register on match">Pattern Number</EWLabel>
          <EWInput value={num} onChange={setNum} placeholder="1" error={!!numErr} type="number" />
          {numErr
            ? <div style={{ fontSize: 10, color: EWC.err, marginTop: 4 }}>{numErr}</div>
            : num !== String(origNum) && num !== '' && <div style={{ fontSize: 10, color: EWC.warn, marginTop: 4, fontFamily: EWmono }}>← was: {origNum}</div>}
        </div>

        <div style={{ height: 1, background: EWC.bd }} />

        {/* current identity chip */}
        <div>
          <EWLabel>Current Identity</EWLabel>
          <div style={{ background: EWC.bg, border: `1px solid ${EWC.bd}`, borderRadius: 5, padding: '9px 12px', display: 'flex', flexDirection: 'column', gap: 5 }}>
            <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
              <span style={{ fontSize: 10, color: EWC.txt3 }}>Name</span>
              <span style={{ fontSize: 11, fontFamily: EWmono, color: EWC.txt }}>{origName}</span>
            </div>
            <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
              <span style={{ fontSize: 10, color: EWC.txt3 }}>Number</span>
              <span style={{ fontSize: 11, fontFamily: EWmono, color: '#9cdcfe', fontWeight: 700 }}>#{origNum}</span>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}

/* ══════════ STEP 2 — Pick Point ══════════ */
function StepPick({ pick, setPick }) {
  const onClick = (e) => {
    const r = e.currentTarget.getBoundingClientRect();
    setPick({ x: Math.round(e.clientX - r.left), y: Math.round(e.clientY - r.top) });
  };
  return (
    <div style={{ display: 'flex', gap: 18, height: '100%' }}>
      <div onClick={onClick}
        style={{ flex: 1, background: '#181818', border: `1px solid ${EWC.bd}`, borderRadius: 5, position: 'relative', overflow: 'hidden', cursor: 'crosshair' }}>
        <PartImage />
        {/* crosshair lines */}
        <div style={{ position: 'absolute', left: 0, right: 0, top: pick.y, height: 1, background: `${EWC.warn}cc`, pointerEvents: 'none' }} />
        <div style={{ position: 'absolute', top: 0, bottom: 0, left: pick.x, width: 1, background: `${EWC.warn}cc`, pointerEvents: 'none' }} />
        {/* pick circle */}
        <div style={{ position: 'absolute', left: pick.x - 9, top: pick.y - 9, width: 18, height: 18, border: `1.5px solid ${EWC.warn}`, borderRadius: '50%', pointerEvents: 'none', background: `${EWC.warn}22` }} />
        {/* coord label */}
        <div style={{ position: 'absolute', left: pick.x + 12, top: pick.y - 22, fontSize: 9, fontFamily: EWmono, fontWeight: 700, color: EWC.warn, background: '#000a', padding: '2px 5px', borderRadius: 2, pointerEvents: 'none' }}>
          PICK ({pick.x}, {pick.y})
        </div>
        <div style={{ position: 'absolute', bottom: 8, left: 10, fontSize: 9, color: EWC.txt3, fontFamily: EWmono }}>
          Click anywhere to reposition the picking point
        </div>
      </div>

      <div style={{ width: 280, display: 'flex', flexDirection: 'column', gap: 14 }}>
        <EWLabel>Picking Position (image coords)</EWLabel>
        <div style={{ display: 'grid', gridTemplateColumns: 'auto 1fr', gap: '10px 10px', alignItems: 'center' }}>
          <span style={{ fontSize: 11, color: EWC.acc, fontFamily: EWmono, fontWeight: 700 }}>X</span>
          <EWNum value={pick.x} onChange={v => setPick({ ...pick, x: v })} min={0} max={CW} suffix="px" w="100%" />
          <span style={{ fontSize: 11, color: EWC.acc, fontFamily: EWmono, fontWeight: 700 }}>Y</span>
          <EWNum value={pick.y} onChange={v => setPick({ ...pick, y: v })} min={0} max={CH} suffix="px" w="100%" />
        </div>
        <div style={{ display: 'flex', gap: 6 }}>
          <EWBtn onClick={() => setPick({ x: Math.round(CW / 2), y: Math.round(CH / 2) })}>Center</EWBtn>
          <EWBtn onClick={() => setPick({ x: 280, y: 195 })}>Snap to Boss</EWBtn>
        </div>

        <div style={{ height: 1, background: EWC.bd, margin: '2px 0' }} />
        <div style={{ background: EWC.bg, border: `1px solid ${EWC.bd}`, borderRadius: 5, padding: '10px 12px' }}>
          <div style={{ fontSize: 10, fontWeight: 700, color: EWC.txt3, textTransform: 'uppercase', letterSpacing: '0.08em', marginBottom: 6 }}>How it's used</div>
          <div style={{ fontSize: 11, color: EWC.txt2, lineHeight: 1.5 }}>
            This pattern-relative offset is transformed by the detected pose to produce the real-world TCP target sent to the robot on match.
          </div>
        </div>
      </div>
    </div>
  );
}

/* ══════════ BoxOverlay SVG helper ══════════ */
function BoxOverlay({ pick, box, color, label }) {
  const r = box.angle * Math.PI / 180;
  const cx = pick.x + Math.cos(r) * box.dist;
  const cy = pick.y + Math.sin(r) * box.dist;
  const w = box.w / 2, h = box.h / 2;
  const pts = [[-w, -h], [w, -h], [w, h], [-w, h]].map(([x, y]) => {
    const cr = Math.cos(r), sr = Math.sin(r);
    return [cx + x * cr - y * sr, cy + x * sr + y * cr];
  });
  const path = pts.map((p, i) => (i === 0 ? 'M' : 'L') + p[0].toFixed(1) + ',' + p[1].toFixed(1)).join(' ') + 'Z';
  return (
    <g>
      <line x1={pick.x} y1={pick.y} x2={cx} y2={cy} stroke={color} strokeWidth="1" strokeDasharray="3 3" opacity="0.55" />
      <path d={path} fill={`${color}22`} stroke={color} strokeWidth="1.5" />
      <circle cx={cx} cy={cy} r="2.5" fill={color} />
      <text x={cx} y={cy - h - 4} fill={color} fontSize="9" fontFamily={EWmono} fontWeight="700" textAnchor="middle">{label}</text>
    </g>
  );
}

/* ══════════ STEP 3 — Picking Box ══════════ */
function StepBox({ pick, boxCfg, setBoxCfg }) {
  const boxA = { w: boxCfg.w, h: boxCfg.h, dist: boxCfg.dist, angle: boxCfg.angle };
  const boxB = { w: boxCfg.w, h: boxCfg.h, dist: boxCfg.dist, angle: boxCfg.angle + 180 };
  return (
    <div style={{ display: 'flex', gap: 18, height: '100%' }}>
      <div style={{ flex: 1, background: '#181818', border: `1px solid ${EWC.bd}`, borderRadius: 5, position: 'relative', overflow: 'hidden' }}>
        <PartImage />
        <svg width={CW} height={CH} style={{ position: 'absolute', inset: 0, pointerEvents: 'none' }}>
          <BoxOverlay pick={pick} box={boxA} color="#4ec9b0" label="A" />
          <BoxOverlay pick={pick} box={boxB} color="#4ec9b0" label="B" />
          <line x1={pick.x - 12} y1={pick.y} x2={pick.x + 12} y2={pick.y} stroke="#fff" strokeWidth="1" />
          <line x1={pick.x} y1={pick.y - 12} x2={pick.x} y2={pick.y + 12} stroke="#fff" strokeWidth="1" />
          <circle cx={pick.x} cy={pick.y} r="3" fill="#fff" />
        </svg>
        <div style={{ position: 'absolute', bottom: 8, left: 10, fontSize: 9, color: EWC.txt3, fontFamily: EWmono }}>
          Symmetric gripper jaws — same size, mirrored about the pick point
        </div>
      </div>

      <div style={{ width: 280, display: 'flex', flexDirection: 'column', gap: 12 }}>
        <div style={{ background: `${EWC.acc}1a`, border: `1px solid ${EWC.acc}55`, borderRadius: 5, padding: '8px 11px', fontSize: 10, color: EWC.txt2, lineHeight: 1.5 }}>
          Both jaws share the same dimensions and offset. Box A sits at <strong style={{ color: EWC.acc, fontFamily: EWmono }}>angle</strong>, Box B at <strong style={{ color: EWC.acc, fontFamily: EWmono }}>angle + 180°</strong>.
        </div>

        <EWLabel>Box Size (shared)</EWLabel>
        <div style={{ display: 'grid', gridTemplateColumns: 'auto 1fr', gap: '8px 10px', alignItems: 'center' }}>
          <span style={{ fontSize: 10, color: EWC.txt3, fontFamily: EWmono }}>Width</span>
          <EWNum value={boxCfg.w} onChange={v => setBoxCfg({ ...boxCfg, w: v })} min={5} max={CW} suffix="px" w="100%" />
          <span style={{ fontSize: 10, color: EWC.txt3, fontFamily: EWmono }}>Height</span>
          <EWNum value={boxCfg.h} onChange={v => setBoxCfg({ ...boxCfg, h: v })} min={5} max={CH} suffix="px" w="100%" />
        </div>

        <EWLabel>Offset from Pick Point</EWLabel>
        <div style={{ display: 'grid', gridTemplateColumns: 'auto 1fr', gap: '8px 10px', alignItems: 'center' }}>
          <span style={{ fontSize: 10, color: EWC.txt3, fontFamily: EWmono }}>Distance</span>
          <EWNum value={boxCfg.dist} onChange={v => setBoxCfg({ ...boxCfg, dist: v })} min={0} max={400} suffix="px" w="100%" />
          <span style={{ fontSize: 10, color: EWC.txt3, fontFamily: EWmono }}>Angle</span>
          <EWNum value={boxCfg.angle} onChange={v => setBoxCfg({ ...boxCfg, angle: v })} min={-180} max={180} suffix="°" w="100%" />
        </div>

        <div style={{ display: 'flex', gap: 6 }}>
          <EWBtn onClick={() => setBoxCfg({ w: 120, h: 80, dist: 90, angle: 0 })}>Reset</EWBtn>
          <EWBtn onClick={() => setBoxCfg({ ...boxCfg, angle: boxCfg.angle + 90 })}>Rotate +90°</EWBtn>
        </div>

        <div style={{ height: 1, background: EWC.bd, margin: '2px 0' }} />
        <div style={{ fontSize: 10, color: EWC.txt3, lineHeight: 1.5 }}>
          If a neighbouring object falls inside either box during matching, the candidate is rejected to avoid gripper collisions.
        </div>
      </div>
    </div>
  );
}

/* ══════════ STEP 4 — Finish / Review ══════════ */
function StepFinish({ origName, origNum, origPick, origBox, name, num, pick, boxCfg }) {
  const boxA = { w: boxCfg.w, h: boxCfg.h, dist: boxCfg.dist, angle: boxCfg.angle };
  const boxB = { w: boxCfg.w, h: boxCfg.h, dist: boxCfg.dist, angle: boxCfg.angle + 180 };

  const changes = [];
  if (name !== origName) changes.push({ field: 'Name', from: origName, to: name });
  if (String(num) !== String(origNum)) changes.push({ field: 'Number', from: `#${origNum}`, to: `#${num}` });
  if (pick.x !== origPick.x || pick.y !== origPick.y)
    changes.push({ field: 'Pick Point', from: `(${origPick.x}, ${origPick.y})`, to: `(${pick.x}, ${pick.y})` });
  if (boxCfg.w !== origBox.w || boxCfg.h !== origBox.h)
    changes.push({ field: 'Box Size', from: `${origBox.w} × ${origBox.h}`, to: `${boxCfg.w} × ${boxCfg.h}` });
  if (boxCfg.dist !== origBox.dist)
    changes.push({ field: 'Box Distance', from: `${origBox.dist}px`, to: `${boxCfg.dist}px` });
  if (boxCfg.angle !== origBox.angle)
    changes.push({ field: 'Box Angle', from: `${origBox.angle}°`, to: `${boxCfg.angle}°` });

  const DiffRow = ({ field, from, to }) => (
    <div style={{ display: 'grid', gridTemplateColumns: '80px 1fr 16px 1fr', alignItems: 'center', gap: 6, padding: '5px 0', borderBottom: `1px dashed ${EWC.bd}` }}>
      <span style={{ fontSize: 9, color: EWC.txt3, fontWeight: 700, textTransform: 'uppercase', letterSpacing: '0.06em' }}>{field}</span>
      <span style={{ fontSize: 10, color: EWC.txt3, fontFamily: EWmono, textDecoration: 'line-through', opacity: 0.7 }}>{from}</span>
      <span style={{ fontSize: 11, color: EWC.txt3, textAlign: 'center' }}>→</span>
      <span style={{ fontSize: 10, color: EWC.ok, fontFamily: EWmono, fontWeight: 700 }}>{to}</span>
    </div>
  );

  const StaticRow = ({ k, v }) => (
    <div style={{ display: 'flex', justifyContent: 'space-between', padding: '5px 0', borderBottom: `1px dashed ${EWC.bd}` }}>
      <span style={{ fontSize: 10, color: EWC.txt3, fontWeight: 700, textTransform: 'uppercase', letterSpacing: '0.06em' }}>{k}</span>
      <span style={{ fontSize: 11, color: EWC.txt2, fontFamily: EWmono }}>{v}</span>
    </div>
  );

  return (
    <div style={{ display: 'flex', gap: 18, height: '100%' }}>
      {/* preview with final overlays */}
      <div style={{ flex: 1, background: '#181818', border: `1px solid ${EWC.bd}`, borderRadius: 5, position: 'relative', overflow: 'hidden' }}>
        <PartImage />
        <svg width={CW} height={CH} style={{ position: 'absolute', inset: 0, pointerEvents: 'none' }}>
          <BoxOverlay pick={pick} box={boxA} color="#4ec9b0" label="A" />
          <BoxOverlay pick={pick} box={boxB} color="#4ec9b0" label="B" />
          <line x1={pick.x - 12} y1={pick.y} x2={pick.x + 12} y2={pick.y} stroke="#fff" strokeWidth="1" />
          <line x1={pick.x} y1={pick.y - 12} x2={pick.x} y2={pick.y + 12} stroke="#fff" strokeWidth="1" />
          <circle cx={pick.x} cy={pick.y} r="3" fill={EWC.warn} />
        </svg>
        <div style={{ position: 'absolute', top: 8, left: 8, fontSize: 9, fontFamily: EWmono, fontWeight: 700, color: EWC.ok, background: '#000a', padding: '3px 8px', borderRadius: 3, border: `1px solid ${EWC.ok}55` }}>
          ● CHANGES READY TO SAVE
        </div>
        <div style={{ position: 'absolute', top: 8, right: 8 }}>
          <div style={{ background: `${EWC.lock}bb`, border: `1px solid ${EWC.lock}`, borderRadius: 3, padding: '3px 8px', display: 'flex', alignItems: 'center', gap: 5 }}>
            <svg width="9" height="9" viewBox="0 0 24 24" fill="none" stroke="#fff" strokeWidth="2.5" strokeLinecap="round">
              <rect x="3" y="11" width="18" height="11" rx="2" />
              <path d="M7 11V7a5 5 0 0 1 10 0v4" />
            </svg>
            <span style={{ fontSize: 8, fontFamily: EWmono, color: '#fff', fontWeight: 700 }}>IMAGE UNCHANGED</span>
          </div>
        </div>
      </div>

      {/* right column: diff + static summary */}
      <div style={{ width: 280, display: 'flex', flexDirection: 'column', gap: 6, overflowY: 'auto' }}>
        <div style={{ fontSize: 13, fontWeight: 700, color: EWC.txt, marginBottom: 4 }}>
          {name || '(unnamed)'}
          <span style={{ color: EWC.acc, fontFamily: EWmono, marginLeft: 6 }}>#{num || '—'}</span>
        </div>

        {/* changes block */}
        {changes.length > 0 ? (
          <div style={{ marginBottom: 8 }}>
            <div style={{ fontSize: 9, fontWeight: 700, color: EWC.warn, textTransform: 'uppercase', letterSpacing: '0.08em', marginBottom: 6 }}>
              {changes.length} Change{changes.length > 1 ? 's' : ''}
            </div>
            {changes.map(c => <DiffRow key={c.field} {...c} />)}
          </div>
        ) : (
          <div style={{ background: EWC.bg, border: `1px solid ${EWC.bd}`, borderRadius: 5, padding: '9px 12px', fontSize: 11, color: EWC.txt3, marginBottom: 8 }}>
            No changes made — values are identical to the saved pattern.
          </div>
        )}

        {/* static summary */}
        <div style={{ fontSize: 9, fontWeight: 700, color: EWC.txt3, textTransform: 'uppercase', letterSpacing: '0.08em', marginBottom: 2, marginTop: 4 }}>
          Final Values
        </div>
        <StaticRow k="Name" v={name} />
        <StaticRow k="Number" v={`#${num}`} />
        <StaticRow k="Pick Point" v={`(${pick.x}, ${pick.y}) px`} />
        <StaticRow k="Box Size" v={`${boxCfg.w} × ${boxCfg.h} px`} />
        <StaticRow k="Offset" v={`d=${boxCfg.dist} · ${boxCfg.angle}° / ${boxCfg.angle + 180}°`} />
        <StaticRow k="Image" v="Locked · saved" />

        <div style={{ marginTop: 8, background: `${EWC.ok}15`, border: `1px solid ${EWC.ok}55`, borderRadius: 5, padding: '9px 11px', fontSize: 11, color: EWC.txt2, lineHeight: 1.45 }}>
          On <strong style={{ color: EWC.ok }}>Save Changes</strong>, the pattern metadata is updated immediately. No retraining required.
        </div>
      </div>
    </div>
  );
}

/* ══════════ MAIN EDIT WIZARD ══════════ */
function EditPatternWizard({ groupName, pattern, usedNums, usedNames, onConfirm, onClose }) {
  /* usedNums / usedNames should exclude the current pattern's own values */

  const STEPS = [
    { id: 'identity',  label: 'Identity',     sub: 'Rename or renumber' },
    { id: 'pick',      label: 'Pick Point',   sub: 'Reposition picking offset' },
    { id: 'box',       label: 'Picking Box',  sub: 'Adjust gripper bounds' },
    { id: 'finish',    label: 'Finish',       sub: 'Review & save' },
  ];

  const [step, setStep] = useState(0);

  /* identity */
  const [name, setName] = useState(pattern.name || '');
  const [num,  setNum]  = useState(String(pattern.number || ''));

  /* pick point — initialise from stored pick coords */
  const [pick, setPick] = useState({
    x: pattern.pickX ?? 280,
    y: pattern.pickY ?? 195,
  });

  /* picking box */
  const [boxCfg, setBoxCfg] = useState({
    w:    pattern.pickBoxW    ?? 120,
    h:    pattern.pickBoxH    ?? 80,
    dist: pattern.pickBoxDist ?? 90,
    angle:pattern.pickBoxAngle?? 0,
  });

  /* originals for diff */
  const orig = useRef({
    name:  pattern.name,
    num:   pattern.number,
    pick:  { x: pattern.pickX ?? 280, y: pattern.pickY ?? 195 },
    box:   { w: pattern.pickBoxW ?? 120, h: pattern.pickBoxH ?? 80, dist: pattern.pickBoxDist ?? 90, angle: pattern.pickBoxAngle ?? 0 },
  });

  /* validation */
  const trimmed = name.trim();
  const n = parseInt(num) || 0;
  const nameErr = trimmed && usedNames.includes(trimmed) ? 'Name already exists in group' : null;
  const numErr  = num && (usedNums.includes(n) || n < 1)
    ? (n < 1 ? 'Must be ≥ 1' : 'Number already in use') : null;
  const step0Ok = trimmed && num && !nameErr && !numErr;

  const canNext =
    step === 0 ? !!step0Ok :
    step === 1 ? true :
    step === 2 ? true : true;

  const finish = () => onConfirm({
    name: trimmed, number: n,
    pickX: pick.x, pickY: pick.y,
    pickBoxW: boxCfg.w, pickBoxH: boxCfg.h,
    pickBoxDist: boxCfg.dist, pickBoxAngle: boxCfg.angle,
  });

  return (
    <div onClick={onClose}
      style={{ position: 'fixed', inset: 0, background: 'rgba(5,9,18,0.78)', display: 'flex', alignItems: 'center', justifyContent: 'center', zIndex: 2000, backdropFilter: 'blur(3px)' }}>
      <div onClick={e => e.stopPropagation()}
        style={{ width: 920, height: 620, background: EWC.surf2, border: `1px solid ${EWC.bd2}`, borderRadius: 9, overflow: 'hidden', boxShadow: '0 24px 64px rgba(0,0,0,0.6)', display: 'flex', flexDirection: 'column' }}>

        {/* ── header ── */}
        <div style={{ padding: '12px 18px', background: EWC.hd, borderBottom: `1px solid ${EWC.bd}`, display: 'flex', alignItems: 'center', justifyContent: 'space-between' }}>
          <div style={{ display: 'flex', alignItems: 'center', gap: 10 }}>
            {/* edit pencil icon */}
            <div style={{ width: 28, height: 28, borderRadius: 5, background: `${EWC.acc}22`, border: `1px solid ${EWC.acc}55`, display: 'flex', alignItems: 'center', justifyContent: 'center', flexShrink: 0 }}>
              <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke={EWC.acc} strokeWidth="2" strokeLinecap="round">
                <path d="M11 4H4a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h14a2 2 0 0 0 2-2v-7" />
                <path d="M18.5 2.5a2.121 2.121 0 0 1 3 3L12 15l-4 1 1-4 9.5-9.5z" />
              </svg>
            </div>
            <div>
              <div style={{ fontSize: 13, fontWeight: 700, color: EWC.txt }}>Edit Pattern</div>
              <div style={{ fontSize: 10, color: EWC.txt3, marginTop: 2 }}>
                Group: {groupName}
                <span style={{ color: EWC.bd2, margin: '0 6px' }}>·</span>
                Pattern: <span style={{ color: EWC.txt2, fontFamily: EWmono }}>{pattern.name}</span>
                <span style={{ color: EWC.bd2, margin: '0 6px' }}>·</span>
                Step {step + 1} of {STEPS.length} — {STEPS[step].sub}
              </div>
            </div>
          </div>
          <button onClick={onClose}
            style={{ background: 'transparent', border: 'none', color: EWC.txt2, fontSize: 18, cursor: 'pointer', padding: 4, lineHeight: 1 }}>✕</button>
        </div>

        {/* ── step rail ── */}
        <div style={{ display: 'flex', alignItems: 'stretch', background: EWC.bg, borderBottom: `1px solid ${EWC.bd}`, padding: '0 18px' }}>
          {STEPS.map((s, i) => {
            const done = i < step;
            const cur  = i === step;
            const reachable = i <= step || (i === step + 1 && canNext);
            return (
              <button key={s.id} onClick={() => reachable && setStep(i)}
                style={{
                  flex: 1, padding: '10px 8px', display: 'flex', alignItems: 'center', gap: 8,
                  background: 'transparent', border: 'none', cursor: reachable ? 'pointer' : 'not-allowed',
                  borderBottom: `2px solid ${cur ? EWC.acc : 'transparent'}`,
                  fontFamily: EWsans, textAlign: 'left',
                }}>
                <div style={{
                  width: 22, height: 22, borderRadius: '50%', flexShrink: 0,
                  display: 'flex', alignItems: 'center', justifyContent: 'center',
                  fontSize: 10, fontWeight: 700, fontFamily: EWmono,
                  background: done ? EWC.ok : cur ? EWC.acc : EWC.surf,
                  color: (done || cur) ? '#fff' : EWC.txt3,
                  border: `1px solid ${done ? EWC.ok : cur ? EWC.acc : EWC.bd2}`,
                }}>
                  {done ? '✓' : i + 1}
                </div>
                <div style={{ minWidth: 0 }}>
                  <div style={{ fontSize: 11, fontWeight: 600, color: cur ? EWC.txt : done ? EWC.txt2 : EWC.txt3 }}>{s.label}</div>
                  <div style={{ fontSize: 9, color: EWC.txt3, marginTop: 1, whiteSpace: 'nowrap', overflow: 'hidden', textOverflow: 'ellipsis' }}>{s.sub}</div>
                </div>
              </button>
            );
          })}
        </div>

        {/* ── body ── */}
        <div style={{ flex: 1, padding: 18, overflow: 'hidden' }}>
          {step === 0 && (
            <StepIdentity
              origName={orig.current.name} origNum={orig.current.num}
              name={name} setName={setName}
              num={num}   setNum={setNum}
              nameErr={nameErr} numErr={numErr} />
          )}
          {step === 1 && <StepPick pick={pick} setPick={setPick} />}
          {step === 2 && <StepBox pick={pick} boxCfg={boxCfg} setBoxCfg={setBoxCfg} />}
          {step === 3 && (
            <StepFinish
              origName={orig.current.name} origNum={orig.current.num}
              origPick={orig.current.pick}  origBox={orig.current.box}
              name={trimmed} num={n} pick={pick} boxCfg={boxCfg} />
          )}
        </div>

        {/* ── footer ── */}
        <div style={{ padding: '12px 18px', borderTop: `1px solid ${EWC.bd}`, background: EWC.hd, display: 'flex', alignItems: 'center', justifyContent: 'space-between' }}>
          <div style={{ fontSize: 10, color: EWC.txt3, fontFamily: EWmono }}>
            {step === 0 && !step0Ok && (
              !trimmed || !num ? 'Enter a valid name and number to continue.'
              : 'Resolve the highlighted errors.'
            )}
            {step === 0 && step0Ok && `✓ Identity valid — ${trimmed} · #${n}`}
            {step === 1 && `✓ Pick point at (${pick.x}, ${pick.y})`}
            {step === 2 && `✓ Symmetric pair · ${boxCfg.w}×${boxCfg.h} · d=${boxCfg.dist} · ±${boxCfg.angle}°`}
            {step === 3 && '✓ Review complete — ready to save'}
          </div>
          <div style={{ display: 'flex', gap: 8 }}>
            <EWBtn onClick={onClose}>Cancel</EWBtn>
            <EWBtn onClick={() => setStep(s => Math.max(0, s - 1))} disabled={step === 0}>← Back</EWBtn>
            {step < STEPS.length - 1 ? (
              <EWBtn primary onClick={() => canNext && setStep(s => s + 1)} disabled={!canNext}>Next →</EWBtn>
            ) : (
              <EWBtn primary onClick={finish}>
                <svg width="11" height="11" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5" strokeLinecap="round">
                  <polyline points="20 6 9 17 4 12" />
                </svg>
                Save Changes
              </EWBtn>
            )}
          </div>
        </div>

      </div>
    </div>
  );
}

window.EditPatternWizard = EditPatternWizard;
