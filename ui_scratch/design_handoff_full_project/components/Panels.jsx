
// Panels.jsx - Vision, PLC, Camera, Settings panels
const { useState } = React;

/* ─────────── shared ─────────── */
const fieldLabel = { fontSize: 11, color: '#9a9a9a', fontWeight: 600, textTransform: 'uppercase', letterSpacing: '0.07em', marginBottom: 4, display: 'block' };
const fieldInput = {
  width: '100%', background: '#252526', border: '1px solid #454545',
  borderRadius: 5, padding: '7px 10px', color: '#cccccc', fontSize: 12,
  fontFamily: 'JetBrains Mono, monospace', outline: 'none', boxSizing: 'border-box',
};
const sectionTitle = { fontSize: 10, color: '#7a7a7a', fontWeight: 700, textTransform: 'uppercase', letterSpacing: '0.12em', marginBottom: 10 };
const panelCard = { background: '#252526', border: '1px solid #3c3c3c', borderRadius: 8, padding: '14px 18px' };

function PanelRow({ label, children }) {
  return (
    <div style={{ display: 'flex', alignItems: 'center', gap: 12, padding: '6px 0', borderBottom: '1px solid #3c3c3c' }}>
      <div style={{ width: 180, fontSize: 12, color: '#9a9a9a', flexShrink: 0 }}>{label}</div>
      <div style={{ flex: 1 }}>{children}</div>
    </div>
  );
}

function InlineInput({ value, onChange, mono = true, unit }) {
  const [focused, setFocused] = useState(false);
  return (
    <div style={{ display: 'flex', alignItems: 'center', gap: 6 }}>
      <input
        style={{
          ...fieldInput, padding: '5px 8px',
          borderColor: focused ? '#2b8ce8' : '#454545',
          fontFamily: mono ? 'JetBrains Mono, monospace' : 'Space Grotesk, sans-serif',
        }}
        value={value}
        onChange={e => onChange(e.target.value)}
        onFocus={() => setFocused(true)}
        onBlur={() => setFocused(false)}
      />
      {unit && <span style={{ fontSize: 11, color: '#7a7a7a', flexShrink: 0 }}>{unit}</span>}
    </div>
  );
}

function Toggle({ value, onChange }) {
  return (
    <div
      onClick={() => onChange(!value)}
      style={{
        width: 38, height: 20, borderRadius: 10, cursor: 'pointer',
        background: value ? '#2b8ce8' : '#3c3c3c', position: 'relative',
        transition: 'background 0.2s', flexShrink: 0,
      }}
    >
      <div style={{
        position: 'absolute', top: 3, left: value ? 20 : 3,
        width: 14, height: 14, borderRadius: '50%', background: '#fff',
        transition: 'left 0.2s',
      }} />
    </div>
  );
}

/* ─────────── Vision Panel ─────────── */
const PATTERNS = [
  { id: 1, name: 'Pattern_A01', score: 0.967, w: 120, h: 80, active: true },
  { id: 2, name: 'Pattern_A02', score: 0.912, w: 95, h: 60, active: true },
  { id: 3, name: 'Pattern_B01', score: 0.881, w: 140, h: 110, active: false },
  { id: 4, name: 'Pattern_B02', score: 0.845, w: 88, h: 72, active: true },
  { id: 5, name: 'Pattern_C01', score: 0.799, w: 110, h: 90, active: false },
];

function PatternCard({ pattern, selected, onSelect }) {
  return (
    <div
      onClick={() => onSelect(pattern.id)}
      style={{
        border: `1px solid ${selected ? '#2b8ce8' : '#454545'}`,
        background: selected ? '#094771' : '#252526',
        borderRadius: 6, padding: 10, cursor: 'pointer',
        transition: 'all 0.15s',
      }}
    >
      {/* Pattern thumbnail placeholder */}
      <div style={{
        width: '100%', aspectRatio: '4/3', background: '#1e1e1e',
        borderRadius: 4, marginBottom: 8, position: 'relative',
        border: '1px solid #3c3c3c', overflow: 'hidden',
        display: 'flex', alignItems: 'center', justifyContent: 'center',
      }}>
        {/* grid lines */}
        <svg width="100%" height="100%" style={{ position: 'absolute', inset: 0 }}>
          {[1,2,3,4].map(i => <line key={`h${i}`} x1="0" y1={`${i*20}%`} x2="100%" y2={`${i*20}%`} stroke="#3c3c3c" strokeWidth="0.5" />)}
          {[1,2,3,4].map(i => <line key={`v${i}`} x1={`${i*20}%`} y1="0" x2={`${i*20}%`} y2="100%" stroke="#3c3c3c" strokeWidth="0.5" />)}
        </svg>
        {/* reticle */}
        <svg width="40" height="30" style={{ position: 'relative', zIndex: 1 }}>
          <rect x="5" y="3" width="30" height="24" rx="1" fill="none" stroke="#2b8ce8" strokeWidth="1.2" strokeDasharray="4 2" />
          <line x1="20" y1="0" x2="20" y2="30" stroke="#2b8ce844" strokeWidth="0.8" />
          <line x1="0" y1="15" x2="40" y2="15" stroke="#2b8ce844" strokeWidth="0.8" />
          <circle cx="20" cy="15" r="2" fill="#2b8ce8" />
        </svg>
        {!pattern.active && (
          <div style={{
            position: 'absolute', inset: 0, background: 'rgba(11,15,23,0.6)',
            display: 'flex', alignItems: 'center', justifyContent: 'center',
          }}>
            <span style={{ fontSize: 9, color: '#9a9a9a', fontWeight: 600, letterSpacing: '0.08em' }}>DISABLED</span>
          </div>
        )}
      </div>
      <div style={{ fontSize: 11, color: '#cccccc', fontWeight: 600, fontFamily: 'JetBrains Mono, monospace', marginBottom: 3, overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap' }}>{pattern.name}</div>
      <div style={{ display: 'flex', justifyContent: 'space-between' }}>
        <span style={{ fontSize: 10, color: '#7a7a7a' }}>{pattern.w}×{pattern.h}px</span>
        <span style={{ fontSize: 10, color: pattern.score > 0.9 ? '#22d17a' : pattern.score > 0.8 ? '#f5a623' : '#e84040', fontFamily: 'JetBrains Mono, monospace' }}>{pattern.score.toFixed(3)}</span>
      </div>
    </div>
  );
}

function VisionPanel() {
  const [selectedPattern, setSelectedPattern] = useState(1);
  const [params, setParams] = useState({ threshold: '0.75', maxResults: '5', angleRange: '360', scaleTol: '0.05', overlap: '0.3' });
  const p = v => (k) => setParams(prev => ({ ...prev, [k]: v }));

  return (
    <div style={{ display: 'flex', height: '100%', gap: 0, overflow: 'hidden' }}>
      {/* Left: camera feed */}
      <div style={{ flex: 1, display: 'flex', flexDirection: 'column', padding: '20px 0 20px 24px', gap: 12, minWidth: 0 }}>
        {/* Camera feed area */}
        <div style={{ ...panelCard, flex: 1, position: 'relative', overflow: 'hidden', display: 'flex', flexDirection: 'column' }}>
          <div style={{ ...sectionTitle, marginBottom: 8, display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
            <span>Camera Feed — Basler acA2440</span>
            <div style={{ display: 'flex', gap: 6 }}>
              {['LIVE','CAPTURE','FREEZE'].map(btn => (
                <button key={btn} style={{
                  background: btn === 'LIVE' ? '#2b8ce8' : '#3c3c3c',
                  border: 'none', borderRadius: 4, padding: '3px 10px', fontSize: 10,
                  color: btn === 'LIVE' ? '#fff' : '#9a9a9a', cursor: 'pointer', fontWeight: 700, letterSpacing: '0.06em',
                }}>{btn}</button>
              ))}
            </div>
          </div>
          {/* Camera placeholder */}
          <div style={{
            flex: 1, background: '#181818', borderRadius: 6, position: 'relative',
            border: '1px solid #3c3c3c', overflow: 'hidden', display: 'flex', alignItems: 'center', justifyContent: 'center',
          }}>
            <svg width="100%" height="100%" style={{ position: 'absolute', inset: 0 }}>
              {Array.from({length: 20}, (_,i) => <line key={`h${i}`} x1="0" y1={`${i*5}%`} x2="100%" y2={`${i*5}%`} stroke="#2d2d2d" strokeWidth="0.5"/>)}
              {Array.from({length: 30}, (_,i) => <line key={`v${i}`} x1={`${i*3.33}%`} y1="0" x2={`${i*3.33}%`} y2="100%" stroke="#2d2d2d" strokeWidth="0.5"/>)}
              {/* corner brackets */}
              {[['0%','0%',1,1],['100%','0%',-1,1],['0%','100%',1,-1],['100%','100%',-1,-1]].map(([x,y,sx,sy],i) => (
                <g key={i} transform={`translate(${x === '0%' ? 20 : -20} ${y === '0%' ? 20 : -20})`}>
                  <line x1={x} y1={y} x2={x === '0%' ? 40 : 'calc(100% - 40px)'} y2={y} stroke="#2b8ce8" strokeWidth="1.5" />
                </g>
              ))}
            </svg>
            {/* Corner brackets drawn simply */}
            <div style={{ position: 'absolute', top: 12, left: 12, width: 24, height: 24, borderTop: '2px solid #2b8ce8', borderLeft: '2px solid #2b8ce8' }} />
            <div style={{ position: 'absolute', top: 12, right: 12, width: 24, height: 24, borderTop: '2px solid #2b8ce8', borderRight: '2px solid #2b8ce8' }} />
            <div style={{ position: 'absolute', bottom: 12, left: 12, width: 24, height: 24, borderBottom: '2px solid #2b8ce8', borderLeft: '2px solid #2b8ce8' }} />
            <div style={{ position: 'absolute', bottom: 12, right: 12, width: 24, height: 24, borderBottom: '2px solid #2b8ce8', borderRight: '2px solid #2b8ce8' }} />
            {/* Center reticle */}
            <svg width="80" height="60" style={{ position: 'relative', opacity: 0.6 }}>
              <rect x="5" y="4" width="70" height="52" rx="2" fill="none" stroke="#2b8ce8" strokeWidth="1" strokeDasharray="6 3" />
              <line x1="40" y1="0" x2="40" y2="60" stroke="#2b8ce855" strokeWidth="0.8" />
              <line x1="0" y1="30" x2="80" y2="30" stroke="#2b8ce855" strokeWidth="0.8" />
              <circle cx="40" cy="30" r="4" fill="none" stroke="#2b8ce8" strokeWidth="1.5" />
              <circle cx="40" cy="30" r="1.5" fill="#2b8ce8" />
            </svg>
            <div style={{ position: 'absolute', bottom: 10, left: 14, fontFamily: 'JetBrains Mono, monospace', fontSize: 10, color: '#7a7a7a' }}>
              2448 × 2048 px · 8-bit · Mono
            </div>
            <div style={{ position: 'absolute', bottom: 10, right: 14, fontFamily: 'JetBrains Mono, monospace', fontSize: 10, color: '#22d17a' }}>
              ● LIVE
            </div>
            <div style={{ position: 'absolute', top: 10, left: 14, fontFamily: 'JetBrains Mono, monospace', fontSize: 10, color: '#f5a623' }}>
              Camera Feed Placeholder
            </div>
          </div>

          {/* Match result bar */}
          <div style={{ display: 'flex', gap: 16, marginTop: 10, alignItems: 'center' }}>
            {[['Match Score','0.967','#22d17a'],['Position X','823 px','#cccccc'],['Position Y','512 px','#cccccc'],['Angle','12.4°','#cccccc'],['Scale','1.002','#cccccc']].map(([k,v,c]) => (
              <div key={k} style={{ display: 'flex', flexDirection: 'column', gap: 2 }}>
                <span style={{ fontSize: 9, color: '#7a7a7a', textTransform: 'uppercase', letterSpacing: '0.07em', fontWeight: 600 }}>{k}</span>
                <span style={{ fontSize: 14, fontWeight: 700, color: c, fontFamily: 'JetBrains Mono, monospace' }}>{v}</span>
              </div>
            ))}
          </div>
        </div>
      </div>

      {/* Right: pattern list + params */}
      <div style={{ width: 280, display: 'flex', flexDirection: 'column', padding: '20px 24px 20px 12px', gap: 12, overflow: 'hidden' }}>
        {/* Pattern library */}
        <div style={{ ...panelCard, flex: 1, overflow: 'hidden', display: 'flex', flexDirection: 'column' }}>
          <div style={{ ...sectionTitle, display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
            <span>Pattern Library</span>
            <button style={{ background: '#3c3c3c', border: '1px solid #454545', borderRadius: 4, padding: '3px 8px', fontSize: 10, color: '#2b8ce8', cursor: 'pointer', display: 'flex', alignItems: 'center', gap: 4 }}>
              <IcoPlus size={11} color="#2b8ce8" /> Add
            </button>
          </div>
          <div style={{ flex: 1, overflowY: 'auto', display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 8 }}>
            {PATTERNS.map(p => <PatternCard key={p.id} pattern={p} selected={selectedPattern === p.id} onSelect={setSelectedPattern} />)}
          </div>
        </div>

        {/* Match params */}
        <div style={{ ...panelCard }}>
          <div style={sectionTitle}>Match Parameters</div>
          <div style={{ display: 'flex', flexDirection: 'column', gap: 2 }}>
            <PanelRow label="Score Threshold">
              <InlineInput value={params.threshold} onChange={v => setParams(p => ({...p, threshold: v}))} unit="0–1" />
            </PanelRow>
            <PanelRow label="Max Results">
              <InlineInput value={params.maxResults} onChange={v => setParams(p => ({...p, maxResults: v}))} />
            </PanelRow>
            <PanelRow label="Angle Range">
              <InlineInput value={params.angleRange} onChange={v => setParams(p => ({...p, angleRange: v}))} unit="°" />
            </PanelRow>
            <PanelRow label="Scale Tolerance">
              <InlineInput value={params.scaleTol} onChange={v => setParams(p => ({...p, scaleTol: v}))} />
            </PanelRow>
            <PanelRow label="Overlap Limit">
              <InlineInput value={params.overlap} onChange={v => setParams(p => ({...p, overlap: v}))} />
            </PanelRow>
          </div>
        </div>
      </div>
    </div>
  );
}

/* ─────────── PLC Panel ─────────── */
const M_REGS = [
  { addr: 'M0',   desc: 'Trigger input',     val: 1, type: 'BIT' },
  { addr: 'M1',   desc: 'Vision busy',       val: 0, type: 'BIT' },
  { addr: 'M2',   desc: 'Vision complete',   val: 1, type: 'BIT' },
  { addr: 'M3',   desc: 'Vision error',      val: 0, type: 'BIT' },
  { addr: 'M100', desc: 'Ready output',      val: 1, type: 'BIT' },
  { addr: 'M101', desc: 'Match OK output',   val: 1, type: 'BIT' },
  { addr: 'M102', desc: 'Match NG output',   val: 0, type: 'BIT' },
];
const D_REGS = [
  { addr: 'D100', desc: 'Position X (0.1mm)',  val: 8230, type: 'WORD' },
  { addr: 'D101', desc: 'Position Y (0.1mm)',  val: 5120, type: 'WORD' },
  { addr: 'D102', desc: 'Angle (0.1°)',        val: 124, type: 'WORD' },
  { addr: 'D103', desc: 'Match score (×1000)', val: 967, type: 'WORD' },
  { addr: 'D104', desc: 'Pattern ID',          val: 1,   type: 'WORD' },
  { addr: 'D105', desc: 'Cycle time (ms)',      val: 124, type: 'WORD' },
];

function RegTable({ regs, type }) {
  const [rows, setRows] = useState(regs);
  const isBit = type === 'BIT';
  return (
    <table style={{ width: '100%', borderCollapse: 'collapse', fontFamily: 'JetBrains Mono, monospace', fontSize: 11 }}>
      <thead>
        <tr style={{ borderBottom: '1px solid #454545' }}>
          {['Address', 'Description', 'Value', 'Action'].map(h => (
            <th key={h} style={{ padding: '6px 10px', textAlign: 'left', fontSize: 10, color: '#7a7a7a', fontWeight: 700, textTransform: 'uppercase', letterSpacing: '0.08em' }}>{h}</th>
          ))}
        </tr>
      </thead>
      <tbody>
        {rows.map((row, i) => {
          const isOn = isBit ? row.val === 1 : null;
          return (
            <tr key={row.addr} style={{ borderBottom: '1px solid #252526', background: i % 2 === 0 ? 'transparent' : '#252526' }}>
              <td style={{ padding: '7px 10px', color: '#9cdcfe', fontWeight: 600 }}>{row.addr}</td>
              <td style={{ padding: '7px 10px', color: '#bfbfbf' }}>{row.desc}</td>
              <td style={{ padding: '7px 10px' }}>
                {isBit ? (
                  <span style={{
                    display: 'inline-block', padding: '2px 8px', borderRadius: 3,
                    fontSize: 10, fontWeight: 700, letterSpacing: '0.06em',
                    background: isOn ? '#22d17a22' : '#3c3c3c',
                    color: isOn ? '#22d17a' : '#7a7a7a',
                    border: `1px solid ${isOn ? '#22d17a44' : '#454545'}`,
                  }}>{isOn ? 'ON' : 'OFF'}</span>
                ) : (
                  <span style={{ color: '#cccccc' }}>{row.val}</span>
                )}
              </td>
              <td style={{ padding: '7px 10px' }}>
                {isBit ? (
                  <div style={{ display: 'flex', gap: 4 }}>
                    {['ON','OFF','TOGGLE'].map(cmd => (
                      <button key={cmd} onClick={() => {
                        setRows(prev => prev.map((r, j) => j === i ? {...r, val: cmd === 'ON' ? 1 : cmd === 'OFF' ? 0 : r.val === 1 ? 0 : 1} : r));
                      }} style={{
                        background: '#3c3c3c', border: '1px solid #454545', borderRadius: 3,
                        padding: '2px 7px', fontSize: 10, color: '#9a9a9a', cursor: 'pointer',
                      }}
                        onMouseEnter={e => e.currentTarget.style.borderColor = '#2b8ce8'}
                        onMouseLeave={e => e.currentTarget.style.borderColor = '#454545'}
                      >{cmd}</button>
                    ))}
                  </div>
                ) : (
                  <div style={{ display: 'flex', gap: 6, alignItems: 'center' }}>
                    <input
                      type="number"
                      style={{ ...fieldInput, padding: '3px 6px', width: 80, fontSize: 11 }}
                      value={row.val}
                      onChange={e => setRows(prev => prev.map((r, j) => j === i ? {...r, val: parseInt(e.target.value)||0} : r))}
                    />
                    <button style={{
                      background: '#3c3c3c', border: '1px solid #454545', borderRadius: 3,
                      padding: '3px 8px', fontSize: 10, color: '#9a9a9a', cursor: 'pointer',
                    }}>Write</button>
                  </div>
                )}
              </td>
            </tr>
          );
        })}
      </tbody>
    </table>
  );
}

function PLCPanel({ connected, onToggleConnect }) {
  const [ip, setIp] = useState('192.168.1.20');
  const [port, setPort] = useState('5007');
  const [timeout, setTimeout_] = useState('1000');

  return (
    <div style={{ display: 'flex', flexDirection: 'column', height: '100%', overflowY: 'auto', padding: '20px 24px', gap: 16 }}>
      {/* Connection config */}
      <div style={{ ...panelCard }}>
        <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between', marginBottom: 14 }}>
          <div style={sectionTitle}>PLC Connection — Mitsubishi MC Protocol 3E</div>
          <div style={{ display: 'flex', alignItems: 'center', gap: 10 }}>
            <div style={{ display: 'flex', alignItems: 'center', gap: 6 }}>
              <div style={{ width: 8, height: 8, borderRadius: '50%', background: connected ? '#22d17a' : '#e84040', boxShadow: connected ? '0 0 6px #22d17a' : 'none' }} />
              <span style={{ fontSize: 11, fontWeight: 700, letterSpacing: '0.06em', color: connected ? '#22d17a' : '#e84040' }}>
                {connected ? 'CONNECTED' : 'DISCONNECTED'}
              </span>
            </div>
            <button onClick={onToggleConnect} style={{
              background: connected ? '#2a1a1a' : '#2b8ce8',
              border: `1px solid ${connected ? '#e8404044' : '#2b8ce8'}`,
              borderRadius: 5, padding: '6px 16px', fontSize: 12, fontWeight: 600,
              color: connected ? '#e84040' : '#fff', cursor: 'pointer',
            }}>{connected ? 'Disconnect' : 'Connect'}</button>
          </div>
        </div>
        <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr 1fr', gap: 12 }}>
          <div>
            <label style={fieldLabel}>IP Address</label>
            <input style={fieldInput} value={ip} onChange={e => setIp(e.target.value)} />
          </div>
          <div>
            <label style={fieldLabel}>Port</label>
            <input style={fieldInput} value={port} onChange={e => setPort(e.target.value)} />
          </div>
          <div>
            <label style={fieldLabel}>Timeout (ms)</label>
            <input style={fieldInput} value={timeout} onChange={e => setTimeout_(e.target.value)} />
          </div>
        </div>
      </div>

      {/* M (Bit) device table */}
      <div style={{ ...panelCard }}>
        <div style={{ ...sectionTitle, marginBottom: 12 }}>M Devices — Bit Registers</div>
        <RegTable regs={M_REGS} type="BIT" />
      </div>

      {/* D (Word) device table */}
      <div style={{ ...panelCard }}>
        <div style={{ ...sectionTitle, marginBottom: 12 }}>D Devices — Word Registers</div>
        <RegTable regs={D_REGS} type="WORD" />
      </div>
    </div>
  );
}

/* ─────────── Camera Panel ─────────── */
function CameraPanel() {
  const [exposure, setExposure] = useState('8000');
  const [gain, setGain] = useState('0.0');
  const [triggerMode, setTriggerMode] = useState(true);
  const [autoExposure, setAutoExposure] = useState(false);
  const [selected, setSelected] = useState(0);
  const cameras = [
    { name: 'acA2440-75uc', ip: '192.168.1.10', serial: 'A00123456', status: 'connected', res: '2448×2048', fps: 75 },
    { name: 'acA1920-48gc', ip: '192.168.1.11', serial: 'A00789012', status: 'disconnected', res: '1920×1080', fps: 48 },
  ];
  return (
    <div style={{ display: 'flex', height: '100%', gap: 0, overflow: 'hidden' }}>
      {/* Camera list */}
      <div style={{ width: 260, borderRight: '1px solid #3c3c3c', padding: '20px 16px 20px 24px', display: 'flex', flexDirection: 'column', gap: 8, overflowY: 'auto' }}>
        <div style={{ ...sectionTitle, marginBottom: 12 }}>Available Cameras</div>
        {cameras.map((cam, i) => (
          <div key={i} onClick={() => setSelected(i)} style={{
            border: `1px solid ${selected === i ? '#2b8ce8' : '#454545'}`,
            background: selected === i ? '#094771' : '#252526',
            borderRadius: 7, padding: '12px 14px', cursor: 'pointer', transition: 'all 0.15s',
          }}>
            <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'flex-start', marginBottom: 6 }}>
              <span style={{ fontSize: 12, fontWeight: 700, color: '#cccccc', fontFamily: 'JetBrains Mono, monospace' }}>{cam.name}</span>
              <div style={{ width: 6, height: 6, borderRadius: '50%', background: cam.status === 'connected' ? '#22d17a' : '#e84040', marginTop: 3 }} />
            </div>
            <div style={{ display: 'flex', flexDirection: 'column', gap: 2 }}>
              {[['IP', cam.ip], ['Serial', cam.serial], ['Resolution', cam.res], ['Max FPS', `${cam.fps} fps`]].map(([k,v]) => (
                <div key={k} style={{ display: 'flex', justifyContent: 'space-between' }}>
                  <span style={{ fontSize: 10, color: '#7a7a7a' }}>{k}</span>
                  <span style={{ fontSize: 10, color: '#9a9a9a', fontFamily: 'JetBrains Mono, monospace' }}>{v}</span>
                </div>
              ))}
            </div>
          </div>
        ))}
        <button style={{
          marginTop: 8, background: 'transparent', border: '1px dashed #454545', borderRadius: 7, padding: '10px',
          color: '#7a7a7a', fontSize: 11, cursor: 'pointer', display: 'flex', alignItems: 'center', justifyContent: 'center', gap: 6,
        }}
          onMouseEnter={e => e.currentTarget.style.borderColor = '#2b8ce8'}
          onMouseLeave={e => e.currentTarget.style.borderColor = '#454545'}
        ><IcoPlus size={13} color="#7a7a7a" /> Scan Network</button>
      </div>

      {/* Camera config */}
      <div style={{ flex: 1, padding: '20px 24px', overflowY: 'auto', display: 'flex', flexDirection: 'column', gap: 16 }}>
        <div style={{ ...panelCard }}>
          <div style={sectionTitle}>Acquisition Settings — {cameras[selected].name}</div>
          <div style={{ display: 'flex', flexDirection: 'column', gap: 2 }}>
            <PanelRow label="Exposure Time">
              <div style={{ display: 'flex', alignItems: 'center', gap: 10 }}>
                <InlineInput value={exposure} onChange={setExposure} unit="μs" />
                <Toggle value={autoExposure} onChange={setAutoExposure} />
                <span style={{ fontSize: 11, color: '#9a9a9a' }}>Auto</span>
              </div>
            </PanelRow>
            <PanelRow label="Gain">
              <InlineInput value={gain} onChange={setGain} unit="dB" />
            </PanelRow>
            <PanelRow label="Trigger Mode">
              <div style={{ display: 'flex', alignItems: 'center', gap: 10 }}>
                <Toggle value={triggerMode} onChange={setTriggerMode} />
                <span style={{ fontSize: 11, color: '#9a9a9a' }}>{triggerMode ? 'Hardware trigger (Line1)' : 'Software trigger'}</span>
              </div>
            </PanelRow>
            <PanelRow label="Pixel Format">
              <select style={{ ...fieldInput, padding: '5px 8px' }}>
                <option>Mono8</option><option>Mono12</option><option>BayerRG8</option><option>BayerRG12</option>
              </select>
            </PanelRow>
            <PanelRow label="Binning H/V">
              <div style={{ display: 'flex', gap: 8 }}>
                <select style={{ ...fieldInput, padding: '5px 8px', flex: 1 }}>{[1,2,4].map(v => <option key={v}>{v}×</option>)}</select>
                <select style={{ ...fieldInput, padding: '5px 8px', flex: 1 }}>{[1,2,4].map(v => <option key={v}>{v}×</option>)}</select>
              </div>
            </PanelRow>
          </div>
          <div style={{ display: 'flex', gap: 8, marginTop: 14 }}>
            <button style={{ background: '#2b8ce8', border: 'none', borderRadius: 5, padding: '7px 18px', fontSize: 12, fontWeight: 600, color: '#fff', cursor: 'pointer' }}>Apply</button>
            <button style={{ background: '#3c3c3c', border: '1px solid #454545', borderRadius: 5, padding: '7px 14px', fontSize: 12, color: '#9a9a9a', cursor: 'pointer' }}>Reset Defaults</button>
          </div>
        </div>

        {/* ROI */}
        <div style={{ ...panelCard }}>
          <div style={sectionTitle}>Region of Interest (ROI)</div>
          <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 12 }}>
            {[['Offset X', '0'], ['Offset Y', '0'], ['Width', '2448'], ['Height', '2048']].map(([k,v]) => (
              <div key={k}>
                <label style={fieldLabel}>{k}</label>
                <input style={fieldInput} defaultValue={v} />
              </div>
            ))}
          </div>
        </div>
      </div>
    </div>
  );
}

/* ─────────── Settings Panel ─────────── */
function SettingsPanel({ task }) {
  const [taskName, setTaskName] = useState(task?.name || 'Task_Loc_01');
  const [commDevice, setCommDevice] = useState('PLC_Mitsubishi_01');
  const [outputDevice, setOutputDevice] = useState('PLC_Mitsubishi_01');
  const [retryCount, setRetryCount] = useState('2');
  const [retryDelay, setRetryDelay] = useState('100');
  const [logLevel, setLogLevel] = useState('INFO');

  return (
    <div style={{ display: 'flex', flexDirection: 'column', height: '100%', overflowY: 'auto', padding: '20px 24px', gap: 16, maxWidth: 720 }}>
      <div style={{ ...panelCard }}>
        <div style={sectionTitle}>Task Configuration</div>
        <div style={{ display: 'flex', flexDirection: 'column', gap: 2 }}>
          <PanelRow label="Task Name">
            <InlineInput value={taskName} onChange={setTaskName} mono={false} />
          </PanelRow>
          <PanelRow label="Task Type">
            <select style={{ ...fieldInput, padding: '5px 8px' }}>
              <option>LocalizationTask</option><option>PickPlaceTask</option><option>InspectTask</option>
            </select>
          </PanelRow>
          <PanelRow label="Communication Device">
            <div style={{ display: 'flex', gap: 6 }}>
              <InlineInput value={commDevice} onChange={setCommDevice} />
              <button style={{ background: '#3c3c3c', border: '1px solid #454545', borderRadius: 5, padding: '5px 10px', fontSize: 11, color: '#9a9a9a', cursor: 'pointer', flexShrink: 0 }}>···</button>
            </div>
          </PanelRow>
          <PanelRow label="Output Device">
            <div style={{ display: 'flex', gap: 6 }}>
              <InlineInput value={outputDevice} onChange={setOutputDevice} />
              <button style={{ background: '#3c3c3c', border: '1px solid #454545', borderRadius: 5, padding: '5px 10px', fontSize: 11, color: '#9a9a9a', cursor: 'pointer', flexShrink: 0 }}>···</button>
            </div>
          </PanelRow>
        </div>
      </div>

      <div style={{ ...panelCard }}>
        <div style={sectionTitle}>Execution Settings</div>
        <div style={{ display: 'flex', flexDirection: 'column', gap: 2 }}>
          <PanelRow label="Retry Count">
            <InlineInput value={retryCount} onChange={setRetryCount} unit="times" />
          </PanelRow>
          <PanelRow label="Retry Delay">
            <InlineInput value={retryDelay} onChange={setRetryDelay} unit="ms" />
          </PanelRow>
          <PanelRow label="Log Level">
            <select style={{ ...fieldInput, padding: '5px 8px' }} value={logLevel} onChange={e => setLogLevel(e.target.value)}>
              {['DEBUG','INFO','WARN','ERROR'].map(l => <option key={l}>{l}</option>)}
            </select>
          </PanelRow>
          <PanelRow label="Save Images on NG">
            <Toggle value={true} onChange={() => {}} />
          </PanelRow>
          <PanelRow label="Auto Start on Launch">
            <Toggle value={false} onChange={() => {}} />
          </PanelRow>
        </div>
      </div>

      <div style={{ display: 'flex', gap: 8 }}>
        <button style={{ background: '#2b8ce8', border: 'none', borderRadius: 5, padding: '9px 22px', fontSize: 13, fontWeight: 600, color: '#fff', cursor: 'pointer' }}>Save Settings</button>
        <button style={{ background: 'transparent', border: '1px solid #454545', borderRadius: 5, padding: '9px 16px', fontSize: 13, color: '#9a9a9a', cursor: 'pointer' }}>Discard Changes</button>
      </div>
    </div>
  );
}

/* ─────────── Robot Panel ─────────── */
function RobotPanel() {
  const [ip, setIp] = useState('192.168.1.30');
  const [port, setPort] = useState('18735');
  const [connected, setConnected] = useState(false);
  return (
    <div style={{ display: 'flex', flexDirection: 'column', height: '100%', overflowY: 'auto', padding: '20px 24px', gap: 16, maxWidth: 720 }}>
      <div style={{ ...panelCard }}>
        <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between', marginBottom: 14 }}>
          <div style={sectionTitle}>Robot Controller — R-30iB Plus</div>
          <div style={{ display: 'flex', alignItems: 'center', gap: 10 }}>
            <div style={{ display: 'flex', alignItems: 'center', gap: 6 }}>
              <div style={{ width: 8, height: 8, borderRadius: '50%', background: connected ? '#22d17a' : '#e84040', boxShadow: connected ? '0 0 6px #22d17a' : 'none' }} />
              <span style={{ fontSize: 11, fontWeight: 700, color: connected ? '#22d17a' : '#e84040' }}>{connected ? 'CONNECTED' : 'DISCONNECTED'}</span>
            </div>
            <button onClick={() => setConnected(c => !c)} style={{ background: connected ? '#2a1a1a' : '#2b8ce8', border: `1px solid ${connected ? '#e8404044' : '#2b8ce8'}`, borderRadius: 5, padding: '6px 16px', fontSize: 12, fontWeight: 600, color: connected ? '#e84040' : '#fff', cursor: 'pointer' }}>
              {connected ? 'Disconnect' : 'Connect'}
            </button>
          </div>
        </div>
        <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 12 }}>
          <div><label style={fieldLabel}>IP Address</label><input style={fieldInput} value={ip} onChange={e => setIp(e.target.value)} /></div>
          <div><label style={fieldLabel}>Port</label><input style={fieldInput} value={port} onChange={e => setPort(e.target.value)} /></div>
        </div>
      </div>
      <div style={{ ...panelCard }}>
        <div style={sectionTitle}>Coordinate Mapping</div>
        <div style={{ display: 'flex', flexDirection: 'column', gap: 2 }}>
          {[['Camera → Robot X', '1.000'],['Camera → Robot Y', '1.000'],['Offset X (mm)', '125.0'],['Offset Y (mm)', '0.0'],['Offset Z (mm)', '380.0'],['Rotation (°)', '0.0']].map(([k,v]) => (
            <PanelRow key={k} label={k}><InlineInput value={v} onChange={() => {}} /></PanelRow>
          ))}
        </div>
      </div>
    </div>
  );
}

Object.assign(window, { VisionPanel, PLCPanel, CameraPanel, SettingsPanel, RobotPanel });
