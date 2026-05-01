
// Dialogs.jsx - All modal dialogs
const { useState } = React;

const inputStyle = {
  width:'100%', background:'#0d1420', border:'1px solid #243044',
  borderRadius:5, padding:'8px 11px', color:'#dce8f5', fontSize:13,
  fontFamily:'Space Grotesk, sans-serif', outline:'none', boxSizing:'border-box',
};
const labelStyle = { fontSize:11, color:'#6b7ea0', fontWeight:600, letterSpacing:'0.06em', textTransform:'uppercase', marginBottom:5, display:'block' };
const btnPrimary = { background:'#2b8ce8', border:'none', borderRadius:5, padding:'9px 22px', color:'#fff', fontSize:13, fontWeight:600, cursor:'pointer' };
const btnSecondary = { background:'transparent', border:'1px solid #243044', borderRadius:5, padding:'9px 22px', color:'#6b7ea0', fontSize:13, cursor:'pointer' };

function ModalBackdrop({ children, onClose }) {
  return (
    <div onClick={onClose} style={{
      position:'fixed', inset:0, background:'rgba(5,9,18,0.75)',
      display:'flex', alignItems:'center', justifyContent:'center', zIndex:1000,
      backdropFilter:'blur(3px)',
    }}>
      <div onClick={e=>e.stopPropagation()}>{children}</div>
    </div>
  );
}

function ModalBox({ title, subtitle, children, onClose, width=480 }) {
  return (
    <div style={{ width, background:'#111827', border:'1px solid #1f2d42', borderRadius:10, overflow:'hidden', boxShadow:'0 24px 64px rgba(0,0,0,0.6)' }}>
      <div style={{ padding:'14px 20px', borderBottom:'1px solid #1a2540', display:'flex', alignItems:'center', justifyContent:'space-between', background:'#0e1520' }}>
        <div>
          <div style={{ fontSize:14, fontWeight:700, color:'#dce8f5' }}>{title}</div>
          {subtitle && <div style={{ fontSize:11, color:'#3a4f6a', marginTop:2 }}>{subtitle}</div>}
        </div>
        <button onClick={onClose} style={{ background:'transparent', border:'none', cursor:'pointer', color:'#6b7ea0', padding:'2px', borderRadius:3, display:'flex' }}
          onMouseEnter={e=>e.currentTarget.style.color='#e84040'} onMouseLeave={e=>e.currentTarget.style.color='#6b7ea0'}
        ><IcoX size={16}/></button>
      </div>
      <div style={{ padding:'22px 24px' }}>{children}</div>
    </div>
  );
}

/* ── New Project ── */
function NewProjectDialog({ onConfirm, onClose }) {
  const [name, setName] = useState('');
  const [desc, setDesc] = useState('');
  return (
    <ModalBackdrop onClose={onClose}>
      <ModalBox title="New Project" onClose={onClose}>
        <div style={{ display:'flex', flexDirection:'column', gap:18 }}>
          <div>
            <label style={labelStyle}>Project Name</label>
            <input style={inputStyle} placeholder="e.g. Line A - Picking" value={name} onChange={e=>setName(e.target.value)} autoFocus
              onFocus={e=>e.target.style.borderColor='#2b8ce8'} onBlur={e=>e.target.style.borderColor='#243044'}/>
          </div>
          <div>
            <label style={labelStyle}>Description <span style={{ color:'#3a4f6a', fontWeight:400, textTransform:'none', letterSpacing:0 }}>(optional)</span></label>
            <textarea style={{ ...inputStyle, height:80, resize:'none', lineHeight:1.6 }}
              placeholder="Brief description..." value={desc} onChange={e=>setDesc(e.target.value)}/>
          </div>
          <div style={{ display:'flex', gap:10, justifyContent:'flex-end' }}>
            <button style={btnSecondary} onClick={onClose}>Cancel</button>
            <button style={{ ...btnPrimary, opacity:name.trim()?1:0.45 }} onClick={()=>name.trim()&&onConfirm({ name:name.trim(), description:desc.trim() })}>
              Create Project
            </button>
          </div>
        </div>
      </ModalBox>
    </ModalBackdrop>
  );
}

/* ── New Task ── */
const TASK_TYPES = [
  { value:'LocalizationTask', label:'Localization',  desc:'Pattern-based object localization & pick position' },
  { value:'PickPlaceTask',    label:'Pick & Place',   desc:'Robot pick and place with vision guidance'        },
  { value:'InspectTask',      label:'Inspection',     desc:'Surface and defect inspection via image matching' },
];
function NewTaskDialog({ projectName, onConfirm, onClose }) {
  const [name, setName] = useState('');
  const [type, setType] = useState('LocalizationTask');
  const [path, setPath] = useState('C:/NCR/projects/');
  return (
    <ModalBackdrop onClose={onClose}>
      <ModalBox title="New Task" subtitle={projectName} onClose={onClose} width={520}>
        <div style={{ display:'flex', flexDirection:'column', gap:18 }}>
          <div>
            <label style={labelStyle}>Task Name</label>
            <input style={{ ...inputStyle, fontFamily:'JetBrains Mono, monospace' }}
              placeholder="e.g. Task_Loc_01" value={name} onChange={e=>setName(e.target.value)} autoFocus
              onFocus={e=>e.target.style.borderColor='#2b8ce8'} onBlur={e=>e.target.style.borderColor='#243044'}/>
          </div>
          <div>
            <label style={labelStyle}>Task Type</label>
            <div style={{ display:'flex', flexDirection:'column', gap:8 }}>
              {TASK_TYPES.map(t => (
                <div key={t.value} onClick={()=>setType(t.value)} style={{
                  display:'flex', alignItems:'flex-start', gap:12, padding:'10px 14px',
                  borderRadius:6, cursor:'pointer',
                  border:`1px solid ${type===t.value?'#2b8ce8':'#1f2d42'}`,
                  background:type===t.value?'#0d1e35':'#0d1420', transition:'all 0.15s',
                }}>
                  <div style={{ width:16, height:16, borderRadius:'50%', flexShrink:0, marginTop:1, border:`2px solid ${type===t.value?'#2b8ce8':'#3a4f6a'}`, background:type===t.value?'#2b8ce8':'transparent', display:'flex', alignItems:'center', justifyContent:'center' }}>
                    {type===t.value && <div style={{ width:6, height:6, borderRadius:'50%', background:'#fff' }}/>}
                  </div>
                  <div>
                    <div style={{ fontSize:13, fontWeight:600, color:type===t.value?'#dce8f5':'#8a9ab5', marginBottom:2 }}>{t.label}</div>
                    <div style={{ fontSize:11, color:'#3a4f6a', lineHeight:1.5 }}>{t.desc}</div>
                  </div>
                </div>
              ))}
            </div>
          </div>
          <div>
            <label style={labelStyle}>Save Path</label>
            <div style={{ display:'flex', gap:8 }}>
              <input style={{ ...inputStyle, fontFamily:'JetBrains Mono, monospace', fontSize:12 }} value={path} onChange={e=>setPath(e.target.value)}/>
              <button style={{ ...btnSecondary, padding:'8px 14px', flexShrink:0, fontSize:12 }}>Browse</button>
            </div>
          </div>
          <div style={{ display:'flex', gap:10, justifyContent:'flex-end' }}>
            <button style={btnSecondary} onClick={onClose}>Cancel</button>
            <button style={{ ...btnPrimary, opacity:name.trim()?1:0.45 }} onClick={()=>name.trim()&&onConfirm({ name:name.trim(), type, path })}>
              Create Task
            </button>
          </div>
        </div>
      </ModalBox>
    </ModalBackdrop>
  );
}

/* ── Add Device ── */
const DEVICE_TYPES = [
  {
    value:'camera', label:'Camera', desc:'Basler GigE / USB3 Vision industrial camera',
    color:'#2b8ce8', Icon: ()=><IcoCamera size={22} color="#2b8ce8"/>,
    defaults: { name:'Basler_cam_01', config:{ ip:'192.168.1.10', port:'3956', serial:'' } }
  },
  {
    value:'plc', label:'PLC', desc:'Mitsubishi / Omron / Siemens via Ethernet protocol',
    color:'#22d17a', Icon: ()=><IcoPLC size={22} color="#22d17a"/>,
    defaults: { name:'PLC_Mitsu_01', config:{ ip:'192.168.1.20', port:'5007', protocol:'MC 3E' } }
  },
  {
    value:'robot', label:'Robot Controller', desc:'Fanuc / KUKA / ABB robot controller',
    color:'#f5a623', Icon: ()=><IcoRobot size={22} color="#f5a623"/>,
    defaults: { name:'Robot_Fanuc_01', config:{ ip:'192.168.1.30', port:'18735' } }
  },
];

function AddDeviceDialog({ taskName, otherTasks, onConfirm, onClose }) {
  const [devType, setDevType] = useState('camera');
  const meta = DEVICE_TYPES.find(d=>d.value===devType);
  const [name, setName] = useState(meta.defaults.name);

  const handleTypeChange = (val) => {
    setDevType(val);
    setName(DEVICE_TYPES.find(d=>d.value===val).defaults.name);
  };

  return (
    <ModalBackdrop onClose={onClose}>
      <ModalBox title="Add Device" subtitle={`Task: ${taskName}`} onClose={onClose} width={540}>
        <div style={{ display:'flex', flexDirection:'column', gap:18 }}>

          {/* Device type picker */}
          <div>
            <label style={labelStyle}>Device Type</label>
            <div style={{ display:'grid', gridTemplateColumns:'repeat(3, 1fr)', gap:8 }}>
              {DEVICE_TYPES.map(dt => {
                const active = devType === dt.value;
                return (
                  <div key={dt.value} onClick={()=>handleTypeChange(dt.value)} style={{
                    border:`1px solid ${active ? dt.color : '#1f2d42'}`,
                    background: active ? `${dt.color}12` : '#0d1420',
                    borderRadius:7, padding:'14px 12px', cursor:'pointer',
                    display:'flex', flexDirection:'column', alignItems:'center', gap:8,
                    transition:'all 0.15s',
                  }}>
                    <dt.Icon/>
                    <div style={{ fontSize:12, fontWeight:600, color: active ? '#dce8f5' : '#6b7ea0', textAlign:'center' }}>{dt.label}</div>
                    <div style={{ fontSize:10, color:'#3a4f6a', textAlign:'center', lineHeight:1.4 }}>{dt.desc}</div>
                  </div>
                );
              })}
            </div>
          </div>

          {/* Device name */}
          <div>
            <label style={labelStyle}>Device Name</label>
            <input style={{ ...inputStyle, fontFamily:'JetBrains Mono, monospace' }}
              value={name} onChange={e=>setName(e.target.value)}
              onFocus={e=>e.target.style.borderColor='#2b8ce8'} onBlur={e=>e.target.style.borderColor='#243044'}/>
          </div>

          {/* Info strip */}
          <div style={{ background:'#0a1018', border:'1px solid #1a2540', borderRadius:6, padding:'10px 14px', display:'flex', alignItems:'center', gap:10 }}>
            <IcoWarning size={14} color="#f5a623"/>
            <span style={{ fontSize:11, color:'#6b7ea0', lineHeight:1.5 }}>
              Device can be configured after adding. You can move it to another task at any time.
            </span>
          </div>

          <div style={{ display:'flex', gap:10, justifyContent:'flex-end' }}>
            <button style={btnSecondary} onClick={onClose}>Cancel</button>
            <button style={{ ...btnPrimary, opacity:name.trim()?1:0.45, background: meta.color }}
              onClick={()=>name.trim()&&onConfirm({ type:devType, name:name.trim(), config:{...meta.defaults.config} })}>
              Add Device
            </button>
          </div>
        </div>
      </ModalBox>
    </ModalBackdrop>
  );
}

/* ── Move Device ── */
function MoveDeviceDialog({ device, currentTaskId, allTasks, onConfirm, onClose }) {
  const [targetTaskId, setTargetTaskId] = useState(null);
  const targets = allTasks.filter(t=>t.id !== currentTaskId);
  return (
    <ModalBackdrop onClose={onClose}>
      <ModalBox title="Move Device" subtitle={device.name} onClose={onClose} width={440}>
        <div style={{ display:'flex', flexDirection:'column', gap:16 }}>
          <div style={{ fontSize:12, color:'#6b7ea0' }}>Select destination task:</div>
          <div style={{ display:'flex', flexDirection:'column', gap:6 }}>
            {targets.length===0 ? (
              <div style={{ padding:'16px', color:'#3a4f6a', fontSize:12, textAlign:'center' }}>No other tasks available.</div>
            ) : targets.map(t=>(
              <div key={t.id} onClick={()=>setTargetTaskId(t.id)} style={{
                padding:'10px 14px', borderRadius:6, cursor:'pointer',
                border:`1px solid ${targetTaskId===t.id?'#2b8ce8':'#1f2d42'}`,
                background:targetTaskId===t.id?'#0d1e35':'#0d1420', transition:'all 0.15s',
                display:'flex', alignItems:'center', gap:10,
              }}>
                <IcoTask size={14} color={targetTaskId===t.id?'#2b8ce8':'#6b7ea0'}/>
                <span style={{ fontSize:13, fontFamily:'JetBrains Mono, monospace', color:targetTaskId===t.id?'#dce8f5':'#8a9ab5', fontWeight:targetTaskId===t.id?600:400 }}>{t.name}</span>
              </div>
            ))}
          </div>
          <div style={{ display:'flex', gap:10, justifyContent:'flex-end' }}>
            <button style={btnSecondary} onClick={onClose}>Cancel</button>
            <button style={{ ...btnPrimary, opacity:targetTaskId?1:0.45 }}
              onClick={()=>targetTaskId&&onConfirm(targetTaskId)}>
              Move Device
            </button>
          </div>
        </div>
      </ModalBox>
    </ModalBackdrop>
  );
}

Object.assign(window, { NewProjectDialog, NewTaskDialog, AddDeviceDialog, MoveDeviceDialog });
