
// TaskWorkspace.jsx - Task panel with device nav + content
const { useState } = React;

const TW_DEVICE_META = {
  camera: { label:'Camera', color:'#2b8ce8' },
  plc:    { label:'PLC',    color:'#22d17a' },
  robot:  { label:'Robot',  color:'#f5a623' },
};
const TW_DEVICE_ICONS = { camera: IcoCamera, plc: IcoPLC, robot: IcoRobot };
const TW_TASK_COLORS  = { LocalizationTask:'#2b8ce8', PickPlaceTask:'#22d17a', InspectTask:'#f5a623' };
const TW_TASK_LABELS  = { LocalizationTask:'Localization', PickPlaceTask:'Pick & Place', InspectTask:'Inspection' };

/* ── Status Lamp ── */
function TWStatusLamp({ label, ok }) {
  const color = ok ? '#22d17a' : '#e84040';
  return (
    <div style={{ display:'flex', flexDirection:'column', alignItems:'center', gap:3 }}>
      <div style={{
        width:28, height:28, borderRadius:'50%',
        background: ok
          ? 'radial-gradient(circle at 35% 35%, #2eff98, #22d17a)'
          : 'radial-gradient(circle at 35% 35%, #ff5b5b, #e84040)',
        boxShadow:`0 0 7px ${color}88`,
        border:`1px solid ${color}44`,
      }}/>
      <span style={{ fontSize:8, fontWeight:700, color:'#2a3a52', letterSpacing:'0.08em', textTransform:'uppercase' }}>{label}</span>
    </div>
  );
}

/* ── Device Nav Item ── */
function TWDeviceNavItem({ device, active, onSelect, onMove, onDelete }) {
  const meta = TW_DEVICE_META[device.type] || TW_DEVICE_META.camera;
  const Icon = TW_DEVICE_ICONS[device.type] || IcoCamera;
  const [hover, setHover] = useState(false);
  return (
    <div
      onClick={() => onSelect(device.id)}
      onMouseEnter={() => setHover(true)}
      onMouseLeave={() => setHover(false)}
      style={{
        display:'flex', alignItems:'center', gap:7,
        padding:'7px 10px', borderRadius:5, cursor:'pointer', marginBottom:1,
        background: active ? `${meta.color}15` : hover ? '#0f1a2a' : 'transparent',
        border:`1px solid ${active ? meta.color+'44' : 'transparent'}`,
        transition:'all 0.13s',
      }}
    >
      <div style={{ position:'relative', flexShrink:0 }}>
        <Icon size={15} color={active ? meta.color : '#2a3a52'}/>
        <div style={{ position:'absolute', bottom:-1, right:-1, width:5, height:5, borderRadius:'50%', background:device.connected?'#22d17a':'#e84040', border:'1px solid #09111e' }}/>
      </div>
      <div style={{ flex:1, minWidth:0 }}>
        <div style={{ fontSize:11, fontFamily:'JetBrains Mono, monospace', fontWeight:active?700:400, color:active?'#dce8f5':'#6b7ea0', overflow:'hidden', textOverflow:'ellipsis', whiteSpace:'nowrap' }}>{device.name}</div>
        <div style={{ fontSize:9, color:meta.color, fontWeight:700, letterSpacing:'0.05em' }}>{meta.label.toUpperCase()}</div>
      </div>
      {(hover||active) && (
        <div style={{ display:'flex', gap:2, flexShrink:0 }}>
          <button onClick={e=>{e.stopPropagation(); onMove(device);}} title="Move to task" style={{ background:'transparent', border:'none', cursor:'pointer', color:'#2a3a52', padding:'2px', borderRadius:3, display:'flex' }}
            onMouseEnter={e=>{e.currentTarget.style.color='#2b8ce8'; e.currentTarget.style.background='#1a2540';}}
            onMouseLeave={e=>{e.currentTarget.style.color='#2a3a52'; e.currentTarget.style.background='transparent';}}
          >
            <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round"><path d="M5 12h14M12 5l7 7-7 7"/></svg>
          </button>
          <button onClick={e=>{e.stopPropagation(); onDelete(device.id);}} title="Remove" style={{ background:'transparent', border:'none', cursor:'pointer', color:'transparent', padding:'2px', borderRadius:3, display:'flex' }}
            onMouseEnter={e=>{e.currentTarget.style.color='#e84040'; e.currentTarget.style.background='#2a1a1a';}}
            onMouseLeave={e=>{e.currentTarget.style.color='transparent'; e.currentTarget.style.background='transparent';}}
          ><IcoX size={11}/></button>
        </div>
      )}
    </div>
  );
}

/* ── Fixed tab button ── */
function TWFixedTab({ label, Icon, active, onClick }) {
  return (
    <button onClick={onClick} style={{
      background:active?'#152035':'transparent', border:'none', borderRadius:5,
      borderLeft:`2px solid ${active?'#2b8ce8':'transparent'}`,
      padding:'6px 8px', cursor:'pointer', display:'flex', alignItems:'center', gap:7,
      width:'100%', color:active?'#2b8ce8':'#2a3a52', transition:'all 0.13s', fontFamily:'Space Grotesk, sans-serif',
    }}
      onMouseEnter={e=>{ if(!active){ e.currentTarget.style.background='#0f1a2a'; e.currentTarget.style.color='#6b7ea0'; }}}
      onMouseLeave={e=>{ if(!active){ e.currentTarget.style.background='transparent'; e.currentTarget.style.color='#2a3a52'; }}}
    >
      <Icon size={14} color="currentColor"/>
      <span style={{ fontSize:11, fontWeight:active?700:500 }}>{label}</span>
    </button>
  );
}

/* ── TaskWorkspace ── */
function TaskWorkspace({ task, allTasks, activeDeviceId, setActiveDeviceId, onAddDevice, onMoveDevice, onDeleteDevice, onToggleDeviceConnect }) {
  const activeDevice = task.devices.find(d=>d.id===activeDeviceId);

  const renderContent = () => {
    if (activeDeviceId==='__settings') return <SettingsPanel task={task}/>;
    if (activeDevice) {
      if (activeDevice.type==='plc')    return <PLCPanel connected={activeDevice.connected} onToggleConnect={()=>onToggleDeviceConnect(task.id, activeDevice.id)}/>;
      if (activeDevice.type==='robot')  return <RobotPanel/>;
      if (activeDevice.type==='camera') return <CameraPanel/>;
    }
    return <Dashboard task={task}
      connections={{ robot:task.devices.some(d=>d.type==='robot'&&d.connected), plc:task.devices.some(d=>d.type==='plc'&&d.connected), camera:task.devices.some(d=>d.type==='camera'&&d.connected) }}
      onToggleConnection={type=>{ const dev=task.devices.find(d=>d.type===type); if(dev) onToggleDeviceConnect(task.id,dev.id); }}/>;
  };

  const breadcrumb = activeDeviceId==='__settings' ? 'Settings' : activeDevice ? activeDevice.name : 'Dashboard';
  const devMeta = activeDevice ? TW_DEVICE_META[activeDevice.type] : null;

  const propContext = activeDevice
    ? { name:activeDevice.name, color:devMeta?.color||'#2b8ce8', typeLabel:devMeta?.label||'' }
    : { name:task.name, color:TW_TASK_COLORS[task.type]||'#2b8ce8', typeLabel:(TW_TASK_LABELS[task.type]||task.type).toUpperCase() };

  return (
    <div style={{ flex:1, display:'flex', overflow:'hidden', minWidth:0 }}>
      {/* Left device nav */}
      <div style={{ width:188, background:'#07101c', borderRight:'1px solid #111f30', display:'flex', flexDirection:'column', flexShrink:0 }}>
        {/* Task header */}
        <div style={{ padding:'10px 12px 8px', borderBottom:'1px solid #111f30' }}>
          <div style={{ fontSize:9, color:'#1f2d42', fontWeight:700, textTransform:'uppercase', letterSpacing:'0.1em', marginBottom:3 }}>Active Task</div>
          <div style={{ fontSize:11, fontFamily:'JetBrains Mono, monospace', fontWeight:700, color:'#dce8f5', overflow:'hidden', textOverflow:'ellipsis', whiteSpace:'nowrap' }}>{task.name}</div>
          <span style={{ marginTop:4, display:'inline-block', fontSize:9, fontWeight:700, color:TW_TASK_COLORS[task.type]||'#6b7ea0', background:`${TW_TASK_COLORS[task.type]||'#6b7ea0'}15`, border:`1px solid ${TW_TASK_COLORS[task.type]||'#6b7ea0'}33`, borderRadius:3, padding:'1px 6px', letterSpacing:'0.05em' }}>
            {TW_TASK_LABELS[task.type]||task.type}
          </span>
        </div>

        {/* Status lamps */}
        <div style={{ padding:'8px 6px', borderBottom:'1px solid #111f30', display:'flex', justifyContent:'space-around' }}>
          <TWStatusLamp label="READY" ok={task.devices.some(d=>d.connected)}/>
          <TWStatusLamp label="CAM"   ok={task.devices.some(d=>d.type==='camera'&&d.connected)}/>
          <TWStatusLamp label="PLC"   ok={task.devices.some(d=>d.type==='plc'&&d.connected)}/>
          <TWStatusLamp label="BOT"   ok={task.devices.some(d=>d.type==='robot'&&d.connected)}/>
        </div>

        {/* Dashboard tab */}
        <div style={{ padding:'6px 8px 2px' }}>
          <TWFixedTab label="Dashboard" Icon={IcoDashboard} active={!activeDeviceId} onClick={()=>setActiveDeviceId(null)}/>
        </div>

        {/* Devices section */}
        <div style={{ padding:'6px 10px 3px', display:'flex', alignItems:'center', justifyContent:'space-between' }}>
          <span style={{ fontSize:9, fontWeight:700, color:'#1a2a3a', textTransform:'uppercase', letterSpacing:'0.1em' }}>Devices</span>
          <button onClick={()=>onAddDevice(task.id)} style={{ background:'transparent', border:'none', cursor:'pointer', color:'#1f2d42', padding:'2px', borderRadius:3, display:'flex' }}
            onMouseEnter={e=>{e.currentTarget.style.color='#2b8ce8'; e.currentTarget.style.background='#1a2540';}}
            onMouseLeave={e=>{e.currentTarget.style.color='#1f2d42'; e.currentTarget.style.background='transparent';}}
          ><IcoPlus size={13}/></button>
        </div>

        <div style={{ flex:1, overflowY:'auto', padding:'0 8px' }}>
          {task.devices.length===0 ? (
            <div onClick={()=>onAddDevice(task.id)} style={{ padding:'10px 8px', color:'#1a2a3a', fontSize:11, textAlign:'center', cursor:'pointer', borderRadius:5, border:'1px dashed #111f30', marginTop:4 }}
              onMouseEnter={e=>e.currentTarget.style.borderColor='#2b8ce8'}
              onMouseLeave={e=>e.currentTarget.style.borderColor='#111f30'}
            ><IcoPlus size={14} color="#1a2a3a"/><br/>Add device</div>
          ) : task.devices.map(dev=>(
            <TWDeviceNavItem key={dev.id} device={dev}
              active={activeDeviceId===dev.id}
              onSelect={setActiveDeviceId}
              onMove={device=>onMoveDevice(task.id, device)}
              onDelete={devId=>onDeleteDevice(task.id, devId)}
            />
          ))}
        </div>

        {/* Settings tab */}
        <div style={{ padding:'2px 8px 8px', borderTop:'1px solid #111f30' }}>
          <TWFixedTab label="Settings" Icon={IcoSettings} active={activeDeviceId==='__settings'} onClick={()=>setActiveDeviceId('__settings')}/>
        </div>
      </div>

      {/* Content + breadcrumb */}
      <div style={{ flex:1, display:'flex', flexDirection:'column', overflow:'hidden', minWidth:0 }}>
        {/* Breadcrumb bar */}
        <div style={{ height:36, background:'#0a1118', borderBottom:'1px solid #111f30', display:'flex', alignItems:'center', padding:'0 16px', gap:8, flexShrink:0 }}>
          <span style={{ fontSize:10, color:'#1f2d42', fontFamily:'JetBrains Mono, monospace' }}>{task.name}</span>
          <span style={{ color:'#111f30', fontSize:14 }}>›</span>
          <span style={{ fontSize:11, fontFamily:'JetBrains Mono, monospace', fontWeight:600, color: devMeta?.color||'#5ba3f0' }}>{breadcrumb}</span>
          {activeDevice && <span style={{ fontSize:9, color:activeDevice.connected?'#22d17a':'#e84040', fontWeight:700, letterSpacing:'0.06em' }}>● {activeDevice.connected?'CONNECTED':'DISCONNECTED'}</span>}
          <div style={{ flex:1 }}/>
          <div style={{ display:'flex', gap:4 }}>
            {[IcoSave,IcoRefresh].map((Ico,i)=>(
              <button key={i} style={{ background:'transparent', border:'none', cursor:'pointer', color:'#1f2d42', padding:'4px', borderRadius:4, display:'flex' }}
                onMouseEnter={e=>e.currentTarget.style.color='#dce8f5'} onMouseLeave={e=>e.currentTarget.style.color='#1f2d42'}
              ><Ico size={13}/></button>
            ))}
          </div>
        </div>

        {/* Main content + property panel */}
        <div style={{ flex:1, display:'flex', overflow:'hidden' }}>
          <div style={{ flex:1, overflow:'hidden' }}>{renderContent()}</div>
          <PropertyPanel
            deviceType={activeDevice?.type||null}
            taskType={activeDevice?null:task.type}
            context={propContext}
          />
        </div>
      </div>
    </div>
  );
}

Object.assign(window, { TaskWorkspace });
