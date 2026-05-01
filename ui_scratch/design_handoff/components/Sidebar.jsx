
// Sidebar.jsx - Project tree with device nodes
const { useState } = React;

const DEVICE_META = {
  camera: { label: 'CAM', color: '#2b8ce8' },
  plc:    { label: 'PLC', color: '#22d17a' },
  robot:  { label: 'BOT', color: '#f5a623' },
};
const SIDEBAR_DEVICE_ICONS = { camera: IcoCamera, plc: IcoPLC, robot: IcoRobot };

const TASK_TYPE_COLORS = { LocalizationTask: '#2b8ce8', PickPlaceTask: '#22d17a', InspectTask: '#f5a623' };
const TASK_TYPE_CHIPS  = { LocalizationTask: 'LOC', PickPlaceTask: 'PICK', InspectTask: 'INSP' };

function SidebarBtn({ icon, title, onClick }) {
  return (
    <button title={title} onClick={onClick} style={{
      background:'transparent', border:'none', cursor:'pointer',
      color:'#6b7ea0', padding:'3px', borderRadius:3, display:'flex', alignItems:'center'
    }}
      onMouseEnter={e=>{ e.currentTarget.style.color='#dce8f5'; e.currentTarget.style.background='#1f2d42'; }}
      onMouseLeave={e=>{ e.currentTarget.style.color='#6b7ea0'; e.currentTarget.style.background='transparent'; }}
    >{icon}</button>
  );
}

function DeviceNode({ device, taskId, isActive, onNavigate, onDelete }) {
  const meta = DEVICE_META[device.type] || DEVICE_META.camera;
  return (
    <div
      onClick={() => onNavigate(taskId, device.id)}
      onDoubleClick={() => onNavigate(taskId, device.id)}
      style={{
        display:'flex', alignItems:'center', gap:6,
        padding:'4px 8px 4px 12px', borderRadius:4, cursor:'pointer',
        background: isActive ? '#1a2a45' : 'transparent',
        borderLeft: isActive ? `2px solid ${meta.color}` : '2px solid transparent',
        marginBottom:1, userSelect:'none', transition:'background 0.12s',
      }}
      onMouseEnter={e=>{ if(!isActive) e.currentTarget.style.background='#131c2e'; }}
      onMouseLeave={e=>{ if(!isActive) e.currentTarget.style.background='transparent'; }}
    >
      <span style={{ color: isActive ? meta.color : '#3a4f6a', display:'flex', flexShrink:0 }}>
        {React.createElement(SIDEBAR_DEVICE_ICONS[device.type] || IcoCamera, { size: 13, color: 'currentColor' })}
      </span>
      <span style={{
        flex:1, fontSize:11, fontFamily:'JetBrains Mono, monospace', fontWeight: isActive ? 600 : 400,
        color: isActive ? '#dce8f5' : '#6b7ea0',
        overflow:'hidden', textOverflow:'ellipsis', whiteSpace:'nowrap',
      }}>{device.name}</span>
      <span style={{
        fontSize:8, fontWeight:700, letterSpacing:'0.06em', color: meta.color,
        background:`${meta.color}15`, border:`1px solid ${meta.color}33`,
        borderRadius:3, padding:'1px 4px', flexShrink:0,
      }}>{meta.label}</span>
      <button
        onClick={e=>{ e.stopPropagation(); onDelete(taskId, device.id); }}
        title="Remove device"
        style={{
          background:'transparent', border:'none', cursor:'pointer',
          color:'transparent', padding:'1px', borderRadius:3,
          display:'flex', alignItems:'center', flexShrink:0,
        }}
        onMouseEnter={e=>{ e.currentTarget.style.color='#e84040'; e.currentTarget.style.background='#2a1a1a'; }}
        onMouseLeave={e=>{ e.currentTarget.style.color='transparent'; e.currentTarget.style.background='transparent'; }}
      ><IcoX size={11}/></button>
    </div>
  );
}

function TaskNode({ project, task, activeTaskId, activeDeviceId, onSelectTask, onNavigate, onAddDevice, onDeleteDevice, onAddTask, onDeleteTask }) {
  const [open, setOpen] = useState(true);
  const isActive = activeTaskId === task.id;
  const color = TASK_TYPE_COLORS[task.type] || '#6b7ea0';
  const chip  = TASK_TYPE_CHIPS[task.type]  || 'UNK';

  return (
    <div>
      <div
        onClick={() => { onSelectTask(task.id); setOpen(true); }}
        style={{
          display:'flex', alignItems:'center', gap:5,
          padding:'5px 8px 5px 10px', cursor:'pointer', borderRadius:4,
          background: isActive && !activeDeviceId ? '#1a2a45' : 'transparent',
          borderLeft: isActive && !activeDeviceId ? '2px solid #2b8ce8' : '2px solid transparent',
          userSelect:'none',
        }}
        onMouseEnter={e=>{ if(!(isActive&&!activeDeviceId)) e.currentTarget.style.background='#131c2e'; }}
        onMouseLeave={e=>{ if(!(isActive&&!activeDeviceId)) e.currentTarget.style.background='transparent'; }}
      >
        <span onClick={e=>{ e.stopPropagation(); setOpen(o=>!o); }} style={{ display:'flex', flexShrink:0 }}>
          <IcoChevron size={11} color="#3a4f6a" open={open}/>
        </span>
        <IcoTask size={13} color={isActive && !activeDeviceId ? '#2b8ce8' : '#6b7ea0'}/>
        <span style={{
          flex:1, fontSize:12, fontFamily:'JetBrains Mono, monospace',
          fontWeight: isActive && !activeDeviceId ? 600 : 400,
          color: isActive && !activeDeviceId ? '#dce8f5' : '#8a9ab5',
          overflow:'hidden', textOverflow:'ellipsis', whiteSpace:'nowrap',
        }}>{task.name}</span>
        <span style={{
          fontSize:8, fontWeight:700, color, background:`${color}12`,
          border:`1px solid ${color}33`, borderRadius:3, padding:'1px 4px',
          flexShrink:0, letterSpacing:'0.05em',
        }}>{chip}</span>
        <button
          onClick={e=>{ e.stopPropagation(); onAddDevice(task.id); }}
          title="Add device"
          style={{ background:'transparent', border:'none', cursor:'pointer', color:'#3a4f6a', padding:'2px', borderRadius:3, display:'flex', alignItems:'center', flexShrink:0 }}
          onMouseEnter={e=>{ e.currentTarget.style.color='#2b8ce8'; e.currentTarget.style.background='#1f2d42'; }}
          onMouseLeave={e=>{ e.currentTarget.style.color='#3a4f6a'; e.currentTarget.style.background='transparent'; }}
        ><IcoPlus size={12}/></button>
        <button
          onClick={e=>{ e.stopPropagation(); onDeleteTask(project.id, task.id); }}
          title="Delete task"
          style={{ background:'transparent', border:'none', cursor:'pointer', color:'transparent', padding:'2px', borderRadius:3, display:'flex', alignItems:'center', flexShrink:0 }}
          onMouseEnter={e=>{ e.currentTarget.style.color='#e84040'; e.currentTarget.style.background='#2a1a1a'; }}
          onMouseLeave={e=>{ e.currentTarget.style.color='transparent'; e.currentTarget.style.background='transparent'; }}
        ><IcoTrash size={11}/></button>
      </div>

      {open && (
        <div style={{ paddingLeft:18 }}>
          {task.devices.length === 0 ? (
            <div
              onClick={() => onAddDevice(task.id)}
              style={{
                padding:'5px 10px', fontSize:10, color:'#2a3a52', cursor:'pointer',
                display:'flex', alignItems:'center', gap:5, borderRadius:4,
              }}
              onMouseEnter={e=>e.currentTarget.style.color='#2b8ce8'}
              onMouseLeave={e=>e.currentTarget.style.color='#2a3a52'}
            >
              <IcoPlus size={10} color="currentColor"/> Add device
            </div>
          ) : task.devices.map(dev => (
            <DeviceNode
              key={dev.id}
              device={dev}
              taskId={task.id}
              isActive={activeTaskId === task.id && activeDeviceId === dev.id}
              onNavigate={onNavigate}
              onDelete={onDeleteDevice}
            />
          ))}
        </div>
      )}
    </div>
  );
}

function ProjectNode({ project, activeTaskId, activeDeviceId, onSelectTask, onNavigate, onAddDevice, onDeleteDevice, onAddTask, onDeleteTask }) {
  const [open, setOpen] = useState(true);
  return (
    <div style={{ marginBottom:2 }}>
      <div
        onClick={() => setOpen(o=>!o)}
        style={{
          display:'flex', alignItems:'center', gap:6,
          padding:'5px 10px 5px 12px', cursor:'pointer', borderRadius:4, userSelect:'none',
        }}
        onMouseEnter={e=>e.currentTarget.style.background='#1a2235'}
        onMouseLeave={e=>e.currentTarget.style.background='transparent'}
      >
        <IcoChevron size={12} color="#6b7ea0" open={open}/>
        <IcoFolder size={14} color="#2b8ce8"/>
        <span style={{ flex:1, fontSize:12, fontWeight:600, color:'#dce8f5', overflow:'hidden', textOverflow:'ellipsis', whiteSpace:'nowrap' }}>
          {project.name}
        </span>
        <SidebarBtn icon={<IcoPlus size={13}/>} title="New task" onClick={e=>{ e.stopPropagation(); onAddTask(project.id); }}/>
      </div>
      {open && (
        <div style={{ paddingLeft:8 }}>
          {project.tasks.length === 0 ? (
            <div style={{ padding:'4px 12px', color:'#2a3a52', fontSize:11, fontStyle:'italic' }}>No tasks</div>
          ) : project.tasks.map(task => (
            <TaskNode
              key={task.id}
              project={project}
              task={task}
              activeTaskId={activeTaskId}
              activeDeviceId={activeDeviceId}
              onSelectTask={onSelectTask}
              onNavigate={onNavigate}
              onAddDevice={onAddDevice}
              onDeleteDevice={onDeleteDevice}
              onAddTask={onAddTask}
              onDeleteTask={onDeleteTask}
            />
          ))}
        </div>
      )}
    </div>
  );
}

function Sidebar({ projects, activeTaskId, activeDeviceId, onSelectTask, onNavigate, onAddProject, onAddTask, onDeleteTask, onAddDevice, onDeleteDevice, onOpen, onSave }) {
  return (
    <div style={{
      width:248, minWidth:200, maxWidth:300,
      background:'#0e1520', borderRight:'1px solid #1a2540',
      display:'flex', flexDirection:'column', overflow:'hidden', flexShrink:0,
    }}>
      {/* Header */}
      <div style={{ padding:'10px 12px 8px', borderBottom:'1px solid #1a2540', display:'flex', alignItems:'center', justifyContent:'space-between' }}>
        <span style={{ fontSize:10, fontWeight:700, letterSpacing:'0.1em', color:'#3a4f6a', textTransform:'uppercase' }}>Explorer</span>
        <div style={{ display:'flex', gap:2 }}>
          <SidebarBtn icon={<IcoNew size={14}/>}  title="New project"  onClick={onAddProject}/>
          <SidebarBtn icon={<IcoOpen size={14}/>} title="Open project" onClick={onOpen}/>
          <SidebarBtn icon={<IcoSave size={14}/>} title="Save"         onClick={onSave}/>
        </div>
      </div>

      {/* Hint bar */}
      <div style={{ padding:'5px 12px', borderBottom:'1px solid #111820', background:'#0a0e16' }}>
        <span style={{ fontSize:9, color:'#2a3a52', letterSpacing:'0.04em' }}>
          Double-click device to open · Drag to move
        </span>
      </div>

      {/* Tree */}
      <div style={{ flex:1, overflowY:'auto', padding:'6px 4px' }}>
        {projects.length === 0 ? (
          <div style={{ padding:'20px 16px', color:'#3a4f6a', fontSize:12, textAlign:'center', lineHeight:1.6 }}>
            No projects.<br/>
            <span style={{ color:'#2b8ce8', cursor:'pointer' }} onClick={onAddProject}>Create one</span>
          </div>
        ) : projects.map(proj => (
          <ProjectNode
            key={proj.id}
            project={proj}
            activeTaskId={activeTaskId}
            activeDeviceId={activeDeviceId}
            onSelectTask={onSelectTask}
            onNavigate={onNavigate}
            onAddDevice={onAddDevice}
            onDeleteDevice={onDeleteDevice}
            onAddTask={onAddTask}
            onDeleteTask={onDeleteTask}
          />
        ))}
      </div>

      {/* Footer */}
      <div style={{ padding:'8px 12px', borderTop:'1px solid #1a2540', fontSize:10, color:'#3a4f6a', fontFamily:'JetBrains Mono, monospace', display:'flex', justifyContent:'space-between' }}>
        <span>v2.2.0</span><span>NCR Vision</span>
      </div>
    </div>
  );
}

Object.assign(window, { Sidebar });
