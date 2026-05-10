
// Dashboard.jsx - Task dashboard panel
const { useState, useEffect, useRef } = React;

function StatCard({ label, value, unit, color = '#2b8ce8', sub }) {
  return (
    <div style={{
      background: '#252526', border: '1px solid #3c3c3c', borderRadius: 8,
      padding: '16px 20px', display: 'flex', flexDirection: 'column', gap: 4,
    }}>
      <div style={{ fontSize: 10, color: '#9a9a9a', textTransform: 'uppercase', letterSpacing: '0.08em', fontWeight: 600 }}>{label}</div>
      <div style={{ display: 'flex', alignItems: 'baseline', gap: 4 }}>
        <span style={{ fontSize: 28, fontWeight: 700, color, fontFamily: 'JetBrains Mono, monospace', lineHeight: 1.1 }}>{value}</span>
        {unit && <span style={{ fontSize: 12, color: '#9a9a9a' }}>{unit}</span>}
      </div>
      {sub && <div style={{ fontSize: 11, color: '#7a7a7a' }}>{sub}</div>}
    </div>
  );
}

const LOG_COLORS = { INFO: '#9cdcfe', WARN: '#f5a623', ERROR: '#e84040', OK: '#22d17a' };

function LogRow({ time, level, message }) {
  return (
    <div style={{
      display: 'flex', gap: 12, padding: '5px 0', borderBottom: '1px solid #3c3c3c',
      fontFamily: 'JetBrains Mono, monospace', fontSize: 11, alignItems: 'flex-start',
    }}>
      <span style={{ color: '#7a7a7a', flexShrink: 0 }}>{time}</span>
      <span style={{ color: LOG_COLORS[level] || '#9a9a9a', flexShrink: 0, width: 36 }}>{level}</span>
      <span style={{ color: '#bfbfbf', lineHeight: 1.5 }}>{message}</span>
    </div>
  );
}

const INITIAL_LOGS = [
  { time: '09:41:22', level: 'OK',   message: 'Task initialized successfully' },
  { time: '09:41:23', level: 'INFO', message: 'Camera Basler acA2440 connected — IP 192.168.1.10' },
  { time: '09:41:23', level: 'INFO', message: 'PLC Mitsubishi Q03UDECPU connected via MC Protocol' },
  { time: '09:41:25', level: 'INFO', message: 'Pattern group loaded: 5 patterns' },
  { time: '09:41:30', level: 'OK',   message: 'READY signal sent to PLC (M100=ON)' },
  { time: '09:42:01', level: 'INFO', message: 'Trigger received from PLC (M0=ON)' },
  { time: '09:42:01', level: 'OK',   message: 'Match found — score: 0.967, pos: (823, 512)' },
  { time: '09:42:02', level: 'INFO', message: 'Output written to PLC D100–D105' },
  { time: '09:43:18', level: 'WARN', message: 'Low match score: 0.521 — pattern #3' },
  { time: '09:43:19', level: 'INFO', message: 'Retry #1 — score: 0.882, accepted' },
];

function RunChart({ data }) {
  const W = 240, H = 52;
  const max = Math.max(...data, 1);
  const pts = data.map((v, i) => {
    const x = (i / (data.length - 1)) * W;
    const y = H - (v / max) * H * 0.85 - 4;
    return `${x},${y}`;
  }).join(' ');
  const fill = data.map((v, i) => {
    const x = (i / (data.length - 1)) * W;
    const y = H - (v / max) * H * 0.85 - 4;
    return `${x},${y}`;
  });
  const fillPath = `M ${fill[0]} L ${fill.slice(1).join(' L ')} L ${W},${H} L 0,${H} Z`;
  return (
    <svg width={W} height={H} style={{ overflow: 'visible' }}>
      <defs>
        <linearGradient id="chartGrad" x1="0" y1="0" x2="0" y2="1">
          <stop offset="0%" stopColor="#2b8ce8" stopOpacity="0.3" />
          <stop offset="100%" stopColor="#2b8ce8" stopOpacity="0.01" />
        </linearGradient>
      </defs>
      <path d={fillPath} fill="url(#chartGrad)" />
      <polyline points={pts} fill="none" stroke="#2b8ce8" strokeWidth="1.8" strokeLinecap="round" strokeLinejoin="round" />
    </svg>
  );
}

function DeviceStatusRow({ name, status, detail, onToggle }) {
  const ok = status === 'connected';
  const warn = status === 'warning';
  const color = ok ? '#22d17a' : warn ? '#f5a623' : '#e84040';
  const label = ok ? 'CONNECTED' : warn ? 'WARNING' : 'DISCONNECTED';
  return (
    <div style={{
      display: 'flex', alignItems: 'center', gap: 12,
      padding: '10px 14px', background: '#252526', borderRadius: 6,
      border: `1px solid ${ok ? '#22d17a22' : warn ? '#f5a62322' : '#e8404022'}`,
    }}>
      <div style={{
        width: 8, height: 8, borderRadius: '50%', background: color, flexShrink: 0,
        boxShadow: ok ? `0 0 6px ${color}` : 'none',
      }} />
      <div style={{ flex: 1 }}>
        <div style={{ fontSize: 12, fontWeight: 600, color: '#cccccc' }}>{name}</div>
        <div style={{ fontSize: 10, color: '#7a7a7a', fontFamily: 'JetBrains Mono, monospace', marginTop: 1 }}>{detail}</div>
      </div>
      <div style={{ display: 'flex', alignItems: 'center', gap: 10 }}>
        <span style={{ fontSize: 10, fontWeight: 700, letterSpacing: '0.06em', color }}>{label}</span>
        <button onClick={onToggle} style={{
          background: '#3c3c3c', border: '1px solid #454545', borderRadius: 4,
          padding: '4px 10px', fontSize: 11, color: '#9a9a9a', cursor: 'pointer',
          fontFamily: 'Space Grotesk, sans-serif',
        }}
          onMouseEnter={e => e.currentTarget.style.borderColor = '#2b8ce8'}
          onMouseLeave={e => e.currentTarget.style.borderColor = '#454545'}
        >{ok ? 'Disconnect' : 'Connect'}</button>
      </div>
    </div>
  );
}

function Dashboard({ task, connections, onToggleConnection }) {
  const [logs] = useState(INITIAL_LOGS);
  const chartData = [12,18,15,22,19,25,21,28,24,30,26,22,29,31,27,33,30,28,35,32];
  const successData = [88,91,87,93,90,95,92,88,94,96,91,89,95,97,93,92,96,94,98,95];

  return (
    <div style={{ display: 'flex', flexDirection: 'column', gap: 16, height: '100%', overflowY: 'auto', padding: '20px 24px' }}>
      {/* Stats row */}
      <div style={{ display: 'grid', gridTemplateColumns: 'repeat(4, 1fr)', gap: 12 }}>
        <StatCard label="Total Runs" value="2,847" color="#2b8ce8" sub="Since last reset" />
        <StatCard label="Success Rate" value="96.2" unit="%" color="#22d17a" sub="Last 24h" />
        <StatCard label="Avg Cycle Time" value="124" unit="ms" color="#f5a623" sub="Vision + output" />
        <StatCard label="Last Score" value="0.967" color="#cccccc" sub="Pattern #1" />
      </div>

      {/* Charts row */}
      <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 12 }}>
        <div style={{ background: '#252526', border: '1px solid #3c3c3c', borderRadius: 8, padding: '14px 18px' }}>
          <div style={{ fontSize: 11, color: '#9a9a9a', fontWeight: 600, textTransform: 'uppercase', letterSpacing: '0.08em', marginBottom: 10 }}>
            Runs / min
          </div>
          <RunChart data={chartData} />
          <div style={{ fontSize: 10, color: '#7a7a7a', marginTop: 6, fontFamily: 'JetBrains Mono, monospace' }}>
            avg 28.4 / min · peak 35
          </div>
        </div>
        <div style={{ background: '#252526', border: '1px solid #3c3c3c', borderRadius: 8, padding: '14px 18px' }}>
          <div style={{ fontSize: 11, color: '#9a9a9a', fontWeight: 600, textTransform: 'uppercase', letterSpacing: '0.08em', marginBottom: 10 }}>
            Success Rate %
          </div>
          <RunChart data={successData} />
          <div style={{ fontSize: 10, color: '#7a7a7a', marginTop: 6, fontFamily: 'JetBrains Mono, monospace' }}>
            avg 93.2% · min 87% · max 98%
          </div>
        </div>
      </div>

      {/* Device status */}
      <div>
        <div style={{ fontSize: 11, color: '#9a9a9a', fontWeight: 600, textTransform: 'uppercase', letterSpacing: '0.08em', marginBottom: 10 }}>
          Device Status
        </div>
        <div style={{ display: 'flex', flexDirection: 'column', gap: 6 }}>
          <DeviceStatusRow
            name="Camera — Basler acA2440-75uc"
            status={connections.camera ? 'connected' : 'disconnected'}
            detail="192.168.1.10 · GigE · 2448×2048"
            onToggle={() => onToggleConnection('camera')}
          />
          <DeviceStatusRow
            name="PLC — Mitsubishi Q03UDECPU"
            status={connections.plc ? 'connected' : 'disconnected'}
            detail="192.168.1.20:5007 · MC Protocol 3E"
            onToggle={() => onToggleConnection('plc')}
          />
          <DeviceStatusRow
            name="Robot Controller"
            status={connections.robot ? 'connected' : 'disconnected'}
            detail="192.168.1.30 · R-30iB Plus"
            onToggle={() => onToggleConnection('robot')}
          />
        </div>
      </div>

      {/* System log */}
      <div style={{ background: '#252526', border: '1px solid #3c3c3c', borderRadius: 8, padding: '14px 18px', flex: 1 }}>
        <div style={{ fontSize: 11, color: '#9a9a9a', fontWeight: 600, textTransform: 'uppercase', letterSpacing: '0.08em', marginBottom: 10, display: 'flex', justifyContent: 'space-between' }}>
          <span>System Log</span>
          <button style={{ background: 'transparent', border: 'none', cursor: 'pointer', color: '#9a9a9a', display: 'flex', alignItems: 'center', gap: 4, fontSize: 10, fontWeight: 600, letterSpacing: '0.06em', textTransform: 'uppercase' }}>
            <IcoRefresh size={12} /> Clear
          </button>
        </div>
        <div style={{ maxHeight: 180, overflowY: 'auto' }}>
          {logs.map((l, i) => <LogRow key={i} {...l} />)}
        </div>
      </div>
    </div>
  );
}

Object.assign(window, { Dashboard });
