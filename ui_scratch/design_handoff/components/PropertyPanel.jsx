
// PropertyPanel.jsx - Industrial property inspector panel
const { useState, useRef, useCallback } = React;

/* ── Property schemas per device/context type ── */
const SCHEMAS = {
  camera: [
    { group:'Connection', open:true, props:[
      { key:'ip',      label:'IP Address',    type:'string', value:'192.168.1.10', desc:'GigE camera IP address on the vision network' },
      { key:'port',    label:'Port',          type:'number', value:3956, min:1, max:65535, desc:'Pylon stream port (default 3956)' },
      { key:'serial',  label:'Serial No.',    type:'readonly', value:'A00123456', desc:'Camera serial number (read from device)' },
      { key:'model',   label:'Model',         type:'readonly', value:'acA2440-75uc', desc:'Camera model identifier' },
    ]},
    { group:'Acquisition', open:true, props:[
      { key:'exposure',   label:'Exposure Time',  type:'number', value:8000,  unit:'μs',  min:16, max:1000000, desc:'Sensor exposure time in microseconds' },
      { key:'gain',       label:'Gain',           type:'number', value:0.0,   unit:'dB',  min:0,  max:24,  step:0.1, desc:'Analog gain in dB' },
      { key:'autoExp',    label:'Auto Exposure',  type:'bool',   value:false, desc:'Enable automatic exposure control' },
      { key:'trigger',    label:'Trigger Mode',   type:'select', value:'Hardware', options:['Hardware','Software','Continuous'], desc:'Image acquisition trigger source' },
      { key:'trigSrc',    label:'Trigger Source', type:'select', value:'Line1',    options:['Line1','Line2','Line3','Software'],  desc:'Hardware trigger input line' },
      { key:'pixFmt',     label:'Pixel Format',   type:'select', value:'Mono8',   options:['Mono8','Mono12','BayerRG8','BayerRG12'], desc:'Image pixel encoding format' },
      { key:'fps',        label:'Frame Rate Limit', type:'number', value:75, unit:'fps', min:1, max:200, desc:'Maximum acquisition frame rate' },
    ]},
    { group:'ROI', open:false, props:[
      { key:'roiX', label:'Offset X', type:'number', value:0,    unit:'px', min:0, max:2448, desc:'ROI horizontal offset from sensor origin' },
      { key:'roiY', label:'Offset Y', type:'number', value:0,    unit:'px', min:0, max:2048, desc:'ROI vertical offset from sensor origin' },
      { key:'roiW', label:'Width',    type:'number', value:2448, unit:'px', min:16, max:2448, desc:'ROI width in pixels' },
      { key:'roiH', label:'Height',   type:'number', value:2048, unit:'px', min:16, max:2048, desc:'ROI height in pixels' },
    ]},
    { group:'Advanced', open:false, props:[
      { key:'binH',    label:'Binning H',  type:'select', value:'1×', options:['1×','2×','4×'], desc:'Horizontal pixel binning factor' },
      { key:'binV',    label:'Binning V',  type:'select', value:'1×', options:['1×','2×','4×'], desc:'Vertical pixel binning factor' },
      { key:'revX',    label:'Reverse X',  type:'bool',   value:false, desc:'Mirror image horizontally' },
      { key:'revY',    label:'Reverse Y',  type:'bool',   value:false, desc:'Mirror image vertically' },
      { key:'gamma',   label:'Gamma',      type:'number', value:1.0,  step:0.01, min:0.1, max:4.0, desc:'Gamma correction factor' },
      { key:'black',   label:'Black Level',type:'number', value:0,    unit:'DN', min:0, max:255, desc:'Black level offset in digital numbers' },
    ]},
  ],

  plc: [
    { group:'Connection', open:true, props:[
      { key:'ip',       label:'IP Address',   type:'string', value:'192.168.1.20', desc:'PLC Ethernet interface IP address' },
      { key:'port',     label:'Port',         type:'number', value:5007, min:1, max:65535, desc:'MC Protocol TCP port (default 5007)' },
      { key:'protocol', label:'Protocol',     type:'select', value:'MC 3E', options:['MC 3E','MC 1E','SLMP'], desc:'Mitsubishi communication protocol frame type' },
      { key:'format',   label:'Data Format',  type:'select', value:'Binary', options:['Binary','ASCII'], desc:'MC Protocol data encoding' },
      { key:'timeout',  label:'Timeout',      type:'number', value:1000, unit:'ms', min:100, max:10000, desc:'TCP connection and read timeout in ms' },
      { key:'retry',    label:'Retry Count',  type:'number', value:2,  min:0, max:10, desc:'Number of retries on communication failure' },
    ]},
    { group:'Input Signal Map', open:true, props:[
      { key:'trigBit',  label:'Trigger',      type:'string', value:'M0',   desc:'PLC bit address that triggers vision task' },
      { key:'resetBit', label:'Reset',        type:'string', value:'M1',   desc:'PLC bit address to reset error state' },
      { key:'abortBit', label:'Abort',        type:'string', value:'M2',   desc:'PLC bit to cancel current operation' },
    ]},
    { group:'Output Signal Map', open:true, props:[
      { key:'readyBit',    label:'Ready',        type:'string', value:'M100', desc:'Bit set ON when vision system is ready' },
      { key:'completeBit', label:'Complete',     type:'string', value:'M101', desc:'Bit set ON when result is ready to read' },
      { key:'okBit',       label:'Match OK',     type:'string', value:'M102', desc:'Bit set ON when match score exceeds threshold' },
      { key:'ngBit',       label:'Match NG',     type:'string', value:'M103', desc:'Bit set ON when match fails' },
      { key:'busyBit',     label:'Busy',         type:'string', value:'M104', desc:'Bit set ON during processing' },
    ]},
    { group:'Data Output Registers', open:false, props:[
      { key:'dPosX',  label:'Position X',    type:'string', value:'D100', desc:'Word register for X position output (×0.1mm)' },
      { key:'dPosY',  label:'Position Y',    type:'string', value:'D101', desc:'Word register for Y position output (×0.1mm)' },
      { key:'dAngle', label:'Angle',         type:'string', value:'D102', desc:'Word register for angle output (×0.1°)' },
      { key:'dScore', label:'Match Score',   type:'string', value:'D103', desc:'Word register for match score (×1000)' },
      { key:'dPatID', label:'Pattern ID',    type:'string', value:'D104', desc:'Word register for matched pattern index' },
      { key:'dCycle', label:'Cycle Time',    type:'string', value:'D105', desc:'Word register for cycle time in ms' },
    ]},
  ],

  robot: [
    { group:'Connection', open:true, props:[
      { key:'ip',         label:'IP Address',      type:'string', value:'192.168.1.30', desc:'Robot controller Ethernet IP' },
      { key:'port',       label:'Port',            type:'number', value:18735, min:1, max:65535, desc:'Communication port' },
      { key:'controller', label:'Controller Type', type:'select', value:'R-30iB Plus', options:['R-30iB Plus','R-30iC','KRC4','IRC5','TP3'], desc:'Robot controller model' },
      { key:'timeout',    label:'Timeout',         type:'number', value:2000, unit:'ms', min:100, max:30000, desc:'Connection timeout in ms' },
    ]},
    { group:'Coordinate Transform', open:true, props:[
      { key:'scaleX', label:'Scale X (px→mm)', type:'number', value:0.0823, step:0.0001, desc:'Pixel to mm scale factor for X axis' },
      { key:'scaleY', label:'Scale Y (px→mm)', type:'number', value:0.0823, step:0.0001, desc:'Pixel to mm scale factor for Y axis' },
      { key:'offX',   label:'Offset X',        type:'number', value:125.0, unit:'mm', step:0.1, desc:'Tool center point X offset from camera' },
      { key:'offY',   label:'Offset Y',        type:'number', value:0.0,   unit:'mm', step:0.1, desc:'Tool center point Y offset' },
      { key:'offZ',   label:'Offset Z',        type:'number', value:380.0, unit:'mm', step:0.1, desc:'Camera to robot base Z height' },
      { key:'rot',    label:'Rotation',        type:'number', value:0.0,   unit:'°',  step:0.1, desc:'Angular offset between camera and robot frame' },
    ]},
    { group:'Operation', open:false, props:[
      { key:'approachH', label:'Approach Height', type:'number', value:50.0,  unit:'mm', step:1, desc:'Z height above pick point for approach' },
      { key:'pickH',     label:'Pick Height',     type:'number', value:0.0,   unit:'mm', step:0.5, desc:'Z height at pick contact point' },
      { key:'placeH',    label:'Place Height',    type:'number', value:5.0,   unit:'mm', step:0.5, desc:'Z height at place contact point' },
      { key:'speed',     label:'Speed',           type:'number', value:80,    unit:'%',  min:1, max:100, desc:'Robot motion speed percentage' },
    ]},
  ],

  task_localization: [
    { group:'Matching', open:true, props:[
      { key:'threshold', label:'Score Threshold', type:'number', value:0.75, step:0.01, min:0, max:1, desc:'Minimum match score to accept result (0–1)' },
      { key:'maxResults',label:'Max Results',     type:'number', value:5,    min:1, max:100, desc:'Maximum number of match results to return' },
      { key:'angleRange',label:'Angle Range',     type:'number', value:360,  unit:'°', min:0, max:360, desc:'Search angle range (0=no rotation search)' },
      { key:'angleStp',  label:'Angle Step',      type:'number', value:1.0,  unit:'°', step:0.1, min:0.1, desc:'Angular search step size' },
      { key:'scaleTol',  label:'Scale Tolerance', type:'number', value:0.05, step:0.01, min:0, max:0.5, desc:'Allowed scale deviation from template' },
      { key:'overlap',   label:'Overlap Limit',   type:'number', value:0.3,  step:0.05, min:0, max:1, desc:'Max allowed overlap between matches' },
      { key:'minContrast',label:'Min Contrast',   type:'number', value:20,   min:0, max:255, desc:'Minimum edge contrast for matching' },
    ]},
    { group:'Execution', open:true, props:[
      { key:'retryCount', label:'Retry Count',     type:'number', value:2,    min:0, max:10, desc:'Retries on score below threshold' },
      { key:'retryDelay', label:'Retry Delay',     type:'number', value:100,  unit:'ms', min:0, max:5000, desc:'Wait time between retries' },
      { key:'logLevel',   label:'Log Level',       type:'select', value:'INFO', options:['DEBUG','INFO','WARN','ERROR'], desc:'Minimum log severity level' },
      { key:'saveNG',     label:'Save NG Images',  type:'bool',   value:true,  desc:'Save captured image to disk on NG result' },
      { key:'autoStart',  label:'Auto Start',      type:'bool',   value:false, desc:'Start task automatically on application launch' },
    ]},
    { group:'Calibration', open:false, props:[
      { key:'calScaleX', label:'Scale X (px→mm)', type:'number', value:0.0823, step:0.0001, desc:'Calibrated pixel-to-mm ratio for X' },
      { key:'calScaleY', label:'Scale Y (px→mm)', type:'number', value:0.0823, step:0.0001, desc:'Calibrated pixel-to-mm ratio for Y' },
      { key:'calOffX',   label:'Offset X',        type:'number', value:0.0, unit:'mm', step:0.01, desc:'Coordinate system X offset' },
      { key:'calOffY',   label:'Offset Y',        type:'number', value:0.0, unit:'mm', step:0.01, desc:'Coordinate system Y offset' },
    ]},
  ],

  task_generic: [
    { group:'Execution', open:true, props:[
      { key:'retryCount', label:'Retry Count',    type:'number', value:2,    min:0, max:10, desc:'Retries on failure' },
      { key:'retryDelay', label:'Retry Delay',    type:'number', value:100,  unit:'ms', desc:'Wait time between retries' },
      { key:'logLevel',   label:'Log Level',      type:'select', value:'INFO', options:['DEBUG','INFO','WARN','ERROR'], desc:'Minimum log severity level' },
      { key:'saveNG',     label:'Save NG Images', type:'bool',   value:true,  desc:'Save image on NG result' },
      { key:'autoStart',  label:'Auto Start',     type:'bool',   value:false, desc:'Auto start on launch' },
    ]},
  ],
};

function getSchema(deviceType, taskType) {
  if (deviceType) return SCHEMAS[deviceType] || [];
  if (taskType === 'LocalizationTask') return SCHEMAS.task_localization;
  return SCHEMAS.task_generic;
}

/* ── individual property row ── */
function PropRow({ prop, onChange, onFocus }) {
  const [changed, setChanged] = useState(false);
  const [focused, setFocused] = useState(false);

  const inputBase = {
    background: focused ? '#0a1828' : '#090e18',
    border: `1px solid ${changed ? '#f5a62355' : focused ? '#2b8ce8' : '#1a2540'}`,
    borderRadius: 3, padding: '3px 7px', color: changed ? '#f5a623' : '#dce8f5',
    fontSize: 11, fontFamily: 'JetBrains Mono, monospace',
    outline: 'none', width: '100%', boxSizing: 'border-box', transition: 'all 0.12s',
  };

  const handleChange = (val) => {
    setChanged(true);
    onChange(prop.key, val);
  };

  const renderInput = () => {
    if (prop.type === 'readonly') return (
      <span style={{ fontSize: 11, fontFamily: 'JetBrains Mono, monospace', color: '#3a4f6a', padding: '3px 7px' }}>{prop.value}</span>
    );
    if (prop.type === 'bool') return (
      <div
        onClick={() => handleChange(!prop.value)}
        style={{
          width: 32, height: 17, borderRadius: 9, cursor: 'pointer',
          background: prop.value ? '#2b8ce8' : '#1a2540',
          position: 'relative', transition: 'background 0.18s', flexShrink: 0,
          border: `1px solid ${prop.value ? '#2b8ce8' : '#243044'}`,
        }}
      >
        <div style={{
          position: 'absolute', top: 2, left: prop.value ? 16 : 2,
          width: 11, height: 11, borderRadius: '50%', background: '#fff',
          transition: 'left 0.18s', boxShadow: '0 1px 2px rgba(0,0,0,0.4)',
        }}/>
      </div>
    );
    if (prop.type === 'select') return (
      <select
        value={prop.value}
        onChange={e => handleChange(e.target.value)}
        onFocus={() => { setFocused(true); onFocus(prop); }}
        onBlur={() => setFocused(false)}
        style={{ ...inputBase, paddingRight: 20, cursor: 'pointer' }}
      >
        {prop.options.map(o => <option key={o}>{o}</option>)}
      </select>
    );
    if (prop.type === 'number') return (
      <div style={{ display: 'flex', alignItems: 'center', gap: 4 }}>
        <input
          type="number"
          value={prop.value}
          step={prop.step ?? 1}
          min={prop.min} max={prop.max}
          onChange={e => handleChange(parseFloat(e.target.value) || 0)}
          onFocus={() => { setFocused(true); onFocus(prop); }}
          onBlur={() => setFocused(false)}
          style={{ ...inputBase, flex: 1 }}
        />
        {prop.unit && <span style={{ fontSize: 9, color: '#2a3a52', flexShrink: 0, fontFamily: 'JetBrains Mono, monospace' }}>{prop.unit}</span>}
      </div>
    );
    // string
    return (
      <input
        type="text"
        value={prop.value}
        onChange={e => handleChange(e.target.value)}
        onFocus={() => { setFocused(true); onFocus(prop); }}
        onBlur={() => setFocused(false)}
        style={inputBase}
      />
    );
  };

  return (
    <div style={{
      display: 'grid', gridTemplateColumns: '1fr 1fr',
      padding: '3px 10px 3px 12px',
      background: focused ? '#0d1828' : 'transparent',
      borderLeft: changed ? '2px solid #f5a62355' : '2px solid transparent',
      transition: 'background 0.1s',
      alignItems: 'center', minHeight: 26,
    }}>
      <div style={{
        fontSize: 11, color: focused ? '#8a9ab5' : '#4a6080', lineHeight: 1.2,
        fontWeight: 500, paddingRight: 8, overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap',
      }} title={prop.label}>{prop.label}</div>
      <div style={{ display: 'flex', alignItems: 'center' }}>{renderInput()}</div>
    </div>
  );
}

/* ── group section ── */
function PropGroup({ group, props, onPropChange, onFocus }) {
  const [open, setOpen] = useState(group.open !== false);
  return (
    <div style={{ marginBottom: 2 }}>
      <div
        onClick={() => setOpen(o => !o)}
        style={{
          display: 'flex', alignItems: 'center', gap: 6,
          padding: '5px 10px', cursor: 'pointer', userSelect: 'none',
          background: '#0a111e', borderBottom: '1px solid #111820',
          borderTop: '1px solid #111820',
        }}
        onMouseEnter={e => e.currentTarget.style.background = '#0d1828'}
        onMouseLeave={e => e.currentTarget.style.background = '#0a111e'}
      >
        <IcoChevron size={10} color="#2a3a52" open={open}/>
        <span style={{ fontSize: 10, fontWeight: 700, color: '#3a5070', textTransform: 'uppercase', letterSpacing: '0.1em', flex: 1 }}>
          {group.group}
        </span>
        <span style={{ fontSize: 9, color: '#1f2d42', fontFamily: 'JetBrains Mono, monospace' }}>
          {props.length}
        </span>
      </div>
      {open && (
        <div>
          {props.map(prop => (
            <PropRow key={prop.key} prop={prop} onChange={onPropChange} onFocus={onFocus}/>
          ))}
        </div>
      )}
    </div>
  );
}

/* ── main PropertyPanel ── */
function PropertyPanel({ deviceType, taskType, context }) {
  const [search, setSearch] = useState('');
  const [focusedProp, setFocusedProp] = useState(null);
  const [values, setValues] = useState({});
  const [collapsed, setCollapsed] = useState(false);

  const rawSchema = getSchema(deviceType, taskType);
  const schema = search.trim()
    ? rawSchema.map(g => ({
        ...g,
        open: true,
        props: g.props.filter(p =>
          p.label.toLowerCase().includes(search.toLowerCase()) ||
          p.key.toLowerCase().includes(search.toLowerCase())
        ),
      })).filter(g => g.props.length > 0)
    : rawSchema;

  const handleChange = useCallback((key, val) => {
    setValues(prev => ({ ...prev, [key]: val }));
  }, []);

  const changedCount = Object.keys(values).length;

  const resolvedSchema = schema.map(g => ({
    ...g,
    props: g.props.map(p => ({ ...p, value: values[p.key] !== undefined ? values[p.key] : p.value })),
  }));

  if (collapsed) return (
    <div style={{
      width: 28, background: '#08111e', borderLeft: '1px solid #1a2540',
      display: 'flex', flexDirection: 'column', alignItems: 'center', padding: '10px 0', gap: 8, flexShrink: 0,
    }}>
      <button onClick={() => setCollapsed(false)} title="Open Properties" style={{
        background: 'transparent', border: 'none', cursor: 'pointer', color: '#2a3a52',
        padding: '4px', borderRadius: 4, display: 'flex', writing: 'vertical-rl',
        transform: 'rotate(-90deg)', marginTop: 8,
      }}
        onMouseEnter={e => e.currentTarget.style.color = '#2b8ce8'}
        onMouseLeave={e => e.currentTarget.style.color = '#2a3a52'}
      >
        <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round">
          <line x1="3" y1="6" x2="21" y2="6"/><line x1="3" y1="12" x2="21" y2="12"/><line x1="3" y1="18" x2="21" y2="18"/>
        </svg>
      </button>
      {changedCount > 0 && (
        <div style={{ width: 16, height: 16, borderRadius: '50%', background: '#f5a623', display: 'flex', alignItems: 'center', justifyContent: 'center', fontSize: 9, fontWeight: 700, color: '#0b0f17' }}>
          {changedCount}
        </div>
      )}
    </div>
  );

  return (
    <div style={{
      width: 268, background: '#08111e', borderLeft: '1px solid #1a2540',
      display: 'flex', flexDirection: 'column', flexShrink: 0, overflow: 'hidden',
    }}>
      {/* Header */}
      <div style={{ padding: '8px 10px', borderBottom: '1px solid #111820', background: '#0a111e', display: 'flex', alignItems: 'center', gap: 8, flexShrink: 0 }}>
        <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="#2a3a52" strokeWidth="2" strokeLinecap="round">
          <line x1="3" y1="6" x2="21" y2="6"/><line x1="3" y1="12" x2="21" y2="12"/><line x1="3" y1="18" x2="21" y2="18"/>
        </svg>
        <span style={{ fontSize: 10, fontWeight: 700, color: '#2a3a52', textTransform: 'uppercase', letterSpacing: '0.1em', flex: 1 }}>Properties</span>
        {changedCount > 0 && (
          <span style={{ fontSize: 9, fontWeight: 700, color: '#f5a623', background: '#f5a62322', border: '1px solid #f5a62344', borderRadius: 10, padding: '1px 6px' }}>
            {changedCount} changed
          </span>
        )}
        <button onClick={() => setCollapsed(true)} title="Collapse" style={{ background: 'transparent', border: 'none', cursor: 'pointer', color: '#1f2d42', padding: '2px', borderRadius: 3, display: 'flex' }}
          onMouseEnter={e => e.currentTarget.style.color = '#6b7ea0'}
          onMouseLeave={e => e.currentTarget.style.color = '#1f2d42'}
        ><IcoX size={12}/></button>
      </div>

      {/* Context label */}
      {context && (
        <div style={{ padding: '5px 12px', background: '#060d18', borderBottom: '1px solid #0d1520', display: 'flex', alignItems: 'center', gap: 6 }}>
          <div style={{ width: 5, height: 5, borderRadius: '50%', background: context.color || '#2b8ce8', flexShrink: 0 }}/>
          <span style={{ fontSize: 10, fontFamily: 'JetBrains Mono, monospace', color: '#3a5070', overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap' }}>
            {context.name}
          </span>
          <span style={{ fontSize: 9, color: '#1f2d42', marginLeft: 'auto', fontWeight: 700, letterSpacing: '0.06em' }}>
            {context.typeLabel}
          </span>
        </div>
      )}

      {/* Search */}
      <div style={{ padding: '6px 8px', borderBottom: '1px solid #0d1520', flexShrink: 0 }}>
        <div style={{ position: 'relative' }}>
          <svg width="11" height="11" viewBox="0 0 24 24" fill="none" stroke="#2a3a52" strokeWidth="2.5" strokeLinecap="round" style={{ position: 'absolute', left: 7, top: '50%', transform: 'translateY(-50%)' }}>
            <circle cx="11" cy="11" r="8"/><path d="M21 21l-4.35-4.35"/>
          </svg>
          <input
            value={search}
            onChange={e => setSearch(e.target.value)}
            placeholder="Filter properties…"
            style={{
              width: '100%', background: '#060d18', border: '1px solid #0d1520',
              borderRadius: 4, padding: '5px 8px 5px 24px', color: '#6b7ea0',
              fontSize: 11, fontFamily: 'Space Grotesk, sans-serif', outline: 'none', boxSizing: 'border-box',
            }}
            onFocus={e => e.target.style.borderColor = '#2b8ce8'}
            onBlur={e => e.target.style.borderColor = '#0d1520'}
          />
          {search && (
            <button onClick={() => setSearch('')} style={{ position: 'absolute', right: 6, top: '50%', transform: 'translateY(-50%)', background: 'transparent', border: 'none', cursor: 'pointer', color: '#2a3a52', padding: 0, display: 'flex' }}>
              <IcoX size={10}/>
            </button>
          )}
        </div>
      </div>

      {/* Property tree */}
      <div style={{ flex: 1, overflowY: 'auto' }}>
        {resolvedSchema.length === 0 ? (
          <div style={{ padding: '20px', textAlign: 'center', color: '#1f2d42', fontSize: 11 }}>No properties match</div>
        ) : resolvedSchema.map(g => (
          <PropGroup key={g.group} group={g} props={g.props} onPropChange={handleChange} onFocus={setFocusedProp}/>
        ))}
      </div>

      {/* Description footer */}
      <div style={{ padding: '8px 12px', borderTop: '1px solid #0d1520', background: '#060d18', minHeight: 48, flexShrink: 0 }}>
        {focusedProp ? (
          <>
            <div style={{ fontSize: 10, fontWeight: 700, color: '#2b8ce8', marginBottom: 3, fontFamily: 'JetBrains Mono, monospace' }}>{focusedProp.key}</div>
            <div style={{ fontSize: 10, color: '#3a5070', lineHeight: 1.5 }}>{focusedProp.desc}</div>
          </>
        ) : (
          <div style={{ fontSize: 10, color: '#1f2d42', lineHeight: 1.5 }}>Click a property to see its description</div>
        )}
      </div>

      {/* Apply / Reset */}
      {changedCount > 0 && (
        <div style={{ padding: '8px 10px', borderTop: '1px solid #111820', display: 'flex', gap: 6, flexShrink: 0 }}>
          <button
            onClick={() => setValues({})}
            style={{ flex: 1, background: 'transparent', border: '1px solid #1f2d42', borderRadius: 4, padding: '6px', fontSize: 11, color: '#3a5070', cursor: 'pointer' }}
            onMouseEnter={e => e.currentTarget.style.borderColor = '#2a3a52'}
            onMouseLeave={e => e.currentTarget.style.borderColor = '#1f2d42'}
          >Revert</button>
          <button
            style={{ flex: 2, background: '#2b8ce8', border: 'none', borderRadius: 4, padding: '6px', fontSize: 11, fontWeight: 600, color: '#fff', cursor: 'pointer' }}
          >Apply</button>
        </div>
      )}
    </div>
  );
}

Object.assign(window, { PropertyPanel, SCHEMAS, getSchema });
