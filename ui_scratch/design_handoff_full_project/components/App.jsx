
// App.jsx - Main app state + layout
const { useState, useCallback } = React;

const APP_DEVICE_META = {
  camera: { label:'Camera', color:'#2b8ce8' },
  plc:    { label:'PLC',    color:'#22d17a' },
  robot:  { label:'Robot',  color:'#f5a623' },
};
const APP_TASK_COLORS = { LocalizationTask:'#2b8ce8', PickPlaceTask:'#22d17a', InspectTask:'#f5a623' };
const APP_TASK_LABELS = { LocalizationTask:'Localization', PickPlaceTask:'Pick & Place', InspectTask:'Inspection' };

/* ── Initial data ── */
let _uid = 100;
const nid = () => ++_uid;

const INIT_PROJECTS = [
  {
    id:1, name:'Line A — Bin Picking', description:'Robot bin picking for assembly line A',
    tasks:[
      { id:1, name:'Task_Loc_01', type:'LocalizationTask', devices:[
        { id:'d1', name:'Basler_cam_01', type:'camera', config:{ ip:'192.168.1.10' }, connected:true  },
        { id:'d2', name:'PLC_Mitsu_01',  type:'plc',    config:{ ip:'192.168.1.20' }, connected:true  },
        { id:'d3', name:'Robot_Fanuc_01',type:'robot',  config:{ ip:'192.168.1.30' }, connected:true  },
      ]},
      { id:2, name:'Task_Pick_01', type:'PickPlaceTask', devices:[
        { id:'d4', name:'Basler_cam_02', type:'camera', config:{ ip:'192.168.1.11' }, connected:false },
        { id:'d5', name:'PLC_Mitsu_02',  type:'plc',    config:{ ip:'192.168.1.21' }, connected:true  },
      ]},
    ],
  },
  {
    id:2, name:'Line B — Inspection', description:'Surface defect inspection on painted panels',
    tasks:[
      { id:3, name:'Inspect_Top', type:'InspectTask', devices:[
        { id:'d6', name:'Basler_cam_03', type:'camera', config:{ ip:'192.168.1.12' }, connected:true  },
      ]},
    ],
  },
];

/* ── Navigate screen (project home) ── */
function TaskQuickCard({ task, onClick }) {
  const color = APP_TASK_COLORS[task.type]||'#9a9a9a';
  return (
    <div onClick={onClick} style={{ background:'#252526', border:'1px solid #3c3c3c', borderRadius:8, padding:'16px 18px', cursor:'pointer', transition:'all 0.15s' }}
      onMouseEnter={e=>{e.currentTarget.style.borderColor=color; e.currentTarget.style.background='#2a2d2e';}}
      onMouseLeave={e=>{e.currentTarget.style.borderColor='#3c3c3c'; e.currentTarget.style.background='#252526';}}
    >
      <div style={{ display:'flex', justifyContent:'space-between', alignItems:'flex-start', marginBottom:10 }}>
        <span style={{ fontSize:13, fontWeight:700, color:'#cccccc', fontFamily:'JetBrains Mono, monospace' }}>{task.name}</span>
        <span style={{ fontSize:10, fontWeight:700, color, background:`${color}18`, border:`1px solid ${color}44`, borderRadius:4, padding:'2px 8px' }}>
          {APP_TASK_LABELS[task.type]||task.type}
        </span>
      </div>
      <div style={{ display:'grid', gridTemplateColumns:'repeat(3,1fr)', gap:8, marginBottom:12 }}>
        {[['Devices',task.devices.length],['Connected',task.devices.filter(d=>d.connected).length],['Cameras',task.devices.filter(d=>d.type==='camera').length]].map(([k,v])=>(
          <div key={k} style={{ background:'#1e1e1e', borderRadius:5, padding:'6px 10px' }}>
            <div style={{ fontSize:9, color:'#7a7a7a', textTransform:'uppercase', letterSpacing:'0.07em', fontWeight:600, marginBottom:2 }}>{k}</div>
            <div style={{ fontSize:16, fontWeight:700, color:'#bfbfbf', fontFamily:'JetBrains Mono, monospace' }}>{v}</div>
          </div>
        ))}
      </div>
      <div style={{ display:'flex', gap:5, flexWrap:'wrap' }}>
        {task.devices.map(dev=>{
          const m = APP_DEVICE_META[dev.type]||APP_DEVICE_META.camera;
          return (
            <div key={dev.id} style={{ display:'flex', alignItems:'center', gap:4, background:`${m.color}10`, border:`1px solid ${m.color}33`, borderRadius:4, padding:'2px 7px' }}>
              <div style={{ width:5, height:5, borderRadius:'50%', background:dev.connected?'#22d17a':'#e84040' }}/>
              <span style={{ fontSize:10, fontFamily:'JetBrains Mono, monospace', color:'#9a9a9a' }}>{dev.name}</span>
            </div>
          );
        })}
      </div>
    </div>
  );
}

function NavigateScreen({ project, onSelectTask, onAddTask }) {
  return (
    <div style={{ flex:1, display:'flex', flexDirection:'column', padding:'32px 40px', gap:24, overflowY:'auto' }}>
      <div>
        <div style={{ display:'flex', alignItems:'center', gap:12, marginBottom:6 }}>
          <IcoFolder size={22} color="#2b8ce8"/>
          <h1 style={{ margin:0, fontSize:22, fontWeight:800, color:'#cccccc', letterSpacing:'-0.01em' }}>{project.name}</h1>
        </div>
        {project.description && <p style={{ margin:'0 0 0 34px', fontSize:13, color:'#9a9a9a', lineHeight:1.6 }}>{project.description}</p>}
      </div>
      <div style={{ display:'flex', gap:10 }}>
        {[['Tasks',project.tasks.length,'#2b8ce8'],['Devices',project.tasks.reduce((s,t)=>s+t.devices.length,0),'#22d17a'],['Connected',project.tasks.reduce((s,t)=>s+t.devices.filter(d=>d.connected).length,0),'#f5a623']].map(([k,v,c])=>(
          <div key={k} style={{ background:'#252526', border:'1px solid #3c3c3c', borderRadius:7, padding:'10px 20px', display:'flex', gap:10, alignItems:'center' }}>
            <span style={{ fontSize:22, fontWeight:800, color:c, fontFamily:'JetBrains Mono, monospace' }}>{v}</span>
            <span style={{ fontSize:12, color:'#9a9a9a' }}>{k}</span>
          </div>
        ))}
      </div>
      <div>
        <div style={{ display:'flex', justifyContent:'space-between', alignItems:'center', marginBottom:14 }}>
          <span style={{ fontSize:11, fontWeight:700, color:'#9a9a9a', textTransform:'uppercase', letterSpacing:'0.1em' }}>Tasks</span>
          <button onClick={()=>onAddTask(project.id)} style={{ background:'#2b8ce8', border:'none', borderRadius:5, padding:'7px 14px', fontSize:12, fontWeight:600, color:'#fff', cursor:'pointer', display:'flex', alignItems:'center', gap:6 }}>
            <IcoPlus size={13} color="#fff"/> New Task
          </button>
        </div>
        {project.tasks.length===0 ? (
          <div style={{ background:'#252526', border:'1px dashed #3c3c3c', borderRadius:10, padding:'40px', textAlign:'center', color:'#7a7a7a' }}>
            <div style={{ fontSize:14 }}>No tasks — <span style={{ color:'#2b8ce8', cursor:'pointer' }} onClick={()=>onAddTask(project.id)}>create one</span></div>
          </div>
        ) : (
          <div style={{ display:'grid', gridTemplateColumns:'repeat(auto-fill,minmax(300px,1fr))', gap:12 }}>
            {project.tasks.map(task=><TaskQuickCard key={task.id} task={task} onClick={()=>onSelectTask(task.id)}/>)}
          </div>
        )}
      </div>
    </div>
  );
}

/* ── Empty Home ── */
function EmptyHome({ onNewProject, onOpenProject }) {
  return (
    <div style={{ flex:1, display:'flex', flexDirection:'column', alignItems:'center', justifyContent:'center', gap:16 }}>
      <svg width="56" height="56" viewBox="0 0 24 24" fill="none">
        <rect x="3" y="3" width="8" height="8" rx="1.5" fill="#3c3c3c"/>
        <rect x="13" y="3" width="8" height="8" rx="1.5" fill="#3c3c3c"/>
        <rect x="3" y="13" width="8" height="8" rx="1.5" fill="#3c3c3c"/>
        <circle cx="17" cy="17" r="4" fill="none" stroke="#454545" strokeWidth="1.5"/>
        <circle cx="17" cy="17" r="1.5" fill="#454545"/>
      </svg>
      <div style={{ textAlign:'center' }}>
        <div style={{ fontSize:18, fontWeight:700, color:'#5a5a5a', marginBottom:6 }}>NCR Vision Config</div>
        <div style={{ fontSize:12, color:'#3c3c3c', marginBottom:24 }}>Industrial computer vision configuration platform</div>
        <div style={{ display:'flex', gap:10, justifyContent:'center' }}>
          <button onClick={onNewProject} style={{ background:'#2b8ce8', border:'none', borderRadius:7, padding:'11px 24px', fontSize:13, fontWeight:600, color:'#fff', cursor:'pointer', display:'flex', alignItems:'center', gap:8 }}>
            <IcoNew size={15} color="#fff"/> New Project
          </button>
          <button onClick={onOpenProject} style={{ background:'transparent', border:'1px solid #454545', borderRadius:7, padding:'11px 24px', fontSize:13, fontWeight:600, color:'#9a9a9a', cursor:'pointer', display:'flex', alignItems:'center', gap:8 }}>
            <IcoOpen size={15}/> Open Project
          </button>
        </div>
      </div>
    </div>
  );
}

/* ── Main App ── */
function App() {
  const [projects, setProjects]         = useState(INIT_PROJECTS);
  const [openTaskIds, setOpenTaskIds]   = useState([1]);        // tabs open in dock
  const [activeTaskId, setActiveTaskId] = useState(1);          // active tab
  const [activeProjectId, setActiveProjId] = useState(null);   // project navigate screen
  const [activeDeviceId, setActiveDeviceId] = useState(null);
  const [dlg, setDlg]                   = useState(null);
  const [nextId, setNextId]             = useState(200);

  const allTasks  = projects.flatMap(p=>p.tasks);
  const getTask   = id => allTasks.find(t=>t.id===id);
  const getProj   = id => projects.find(p=>p.id===id);
  const allDevices = allTasks.flatMap(t=>t.devices);

  /* ── Tab management ── */
  const openTab = useCallback(taskId => {
    setOpenTaskIds(prev => prev.includes(taskId) ? prev : [...prev, taskId]);
    setActiveTaskId(taskId);
    setActiveDeviceId(null);
    setActiveProjId(null);
  }, []);

  const selectTask = useCallback(taskId => {
    openTab(taskId);
  }, [openTab]);

  const closeTab = useCallback(taskId => {
    setOpenTaskIds(prev => {
      const next = prev.filter(id=>id!==taskId);
      if (activeTaskId===taskId) setActiveTaskId(next[next.length-1]||null);
      return next;
    });
  }, [activeTaskId]);

  /* ── Project ops ── */
  const addProject = useCallback(({ name, description }) => {
    const id = nextId; setNextId(n=>n+1);
    setProjects(prev=>[...prev, { id, name, description, tasks:[] }]);
    setActiveProjId(id); setActiveTaskId(null); setDlg(null);
  }, [nextId]);

  const addTask = useCallback(({ name, type }) => {
    const id = nextId; setNextId(n=>n+1);
    const task = { id, name, type, devices:[] };
    setProjects(prev=>prev.map(p=>p.id===dlg.projectId?{...p,tasks:[...p.tasks,task]}:p));
    openTab(id); setDlg(null);
  }, [nextId, dlg, openTab]);

  const deleteTask = useCallback((projId, taskId) => {
    setProjects(prev=>prev.map(p=>p.id===projId?{...p,tasks:p.tasks.filter(t=>t.id!==taskId)}:p));
    closeTab(taskId);
  }, [closeTab]);

  /* ── Device ops ── */
  const addDevice = useCallback(({ type, name, config }) => {
    const id = `d${nextId}`; setNextId(n=>n+1);
    const dev = { id, name, type, config, connected:false };
    setProjects(prev=>prev.map(p=>({...p,tasks:p.tasks.map(t=>t.id===dlg.taskId?{...t,devices:[...t.devices,dev]}:t)})));
    setActiveDeviceId(id);
    setDlg(null);
  }, [nextId, dlg]);

  const deleteDevice = useCallback((taskId, devId) => {
    setProjects(prev=>prev.map(p=>({...p,tasks:p.tasks.map(t=>t.id===taskId?{...t,devices:t.devices.filter(d=>d.id!==devId)}:t)})));
    if(activeDeviceId===devId) setActiveDeviceId(null);
  }, [activeDeviceId]);

  const moveDevice = useCallback(targetTaskId => {
    const { taskId, device } = dlg;
    setProjects(prev=>prev.map(p=>({...p,tasks:p.tasks.map(t=>{
      if(t.id===taskId)       return {...t,devices:t.devices.filter(d=>d.id!==device.id)};
      if(t.id===targetTaskId) return {...t,devices:[...t.devices,device]};
      return t;
    })})));
    openTab(targetTaskId); setActiveDeviceId(device.id); setDlg(null);
  }, [dlg, openTab]);

  const toggleDeviceConnect = useCallback((taskId, devId) => {
    setProjects(prev=>prev.map(p=>({...p,tasks:p.tasks.map(t=>t.id===taskId?{...t,devices:t.devices.map(d=>d.id===devId?{...d,connected:!d.connected}:d)}:t)})));
  }, []);

  /* ── Connection summary ── */
  const anyCamera = allDevices.filter(d=>d.type==='camera').some(d=>d.connected);
  const anyPLC    = allDevices.filter(d=>d.type==='plc').some(d=>d.connected);
  const anyRobot  = allDevices.filter(d=>d.type==='robot').some(d=>d.connected);

  /* ── Active project for sidebar navigate ── */
  const activeProject = activeProjectId ? getProj(activeProjectId) : null;
  const showNavigate  = !activeTaskId && activeProject;

  return (
    <div style={{ display:'flex', flexDirection:'column', height:'100vh', background:'#1e1e1e', fontFamily:'Space Grotesk, sans-serif', color:'#cccccc', overflow:'hidden' }}>

      {/* Top bar */}
      <div style={{ height:44, background:'#1e1e1e', borderBottom:'1px solid #2d2d2d', display:'flex', alignItems:'center', padding:'0 16px', gap:0, flexShrink:0 }}>
        <div style={{ display:'flex', alignItems:'center', gap:10, paddingRight:20, borderRight:'1px solid #2d2d2d', marginRight:12 }}>
          <svg width="20" height="20" viewBox="0 0 24 24" fill="none">
            <rect x="3" y="3" width="8" height="8" rx="1.5" fill="#2b8ce8"/>
            <rect x="13" y="3" width="8" height="8" rx="1.5" fill="#2b8ce844"/>
            <rect x="3" y="13" width="8" height="8" rx="1.5" fill="#2b8ce844"/>
            <circle cx="17" cy="17" r="4" fill="none" stroke="#2b8ce8" strokeWidth="1.5"/>
            <circle cx="17" cy="17" r="1.5" fill="#2b8ce8"/>
          </svg>
          <span style={{ fontSize:13, fontWeight:800, color:'#cccccc', letterSpacing:'-0.01em' }}>NCR<span style={{ color:'#2b8ce8' }}>vision</span></span>
        </div>
        {['File','View','Access Level','Help'].map(item=>(
          <button key={item} style={{ background:'transparent', border:'none', cursor:'pointer', padding:'0 12px', height:'100%', fontSize:12, color:'#7a7a7a', fontFamily:'Space Grotesk, sans-serif' }}
            onMouseEnter={e=>e.currentTarget.style.color='#cccccc'} onMouseLeave={e=>e.currentTarget.style.color='#7a7a7a'}
          >{item}</button>
        ))}
        <div style={{ flex:1 }}/>
        <div style={{ display:'flex', gap:6, marginRight:16 }}>
          {[['CAM',anyCamera],['PLC',anyPLC],['ROBOT',anyRobot]].map(([label,ok])=>(
            <div key={label} style={{ display:'flex', alignItems:'center', gap:5, background:ok?'#22d17a0e':'#e840400e', border:`1px solid ${ok?'#22d17a2a':'#e840402a'}`, borderRadius:4, padding:'2px 8px' }}>
              <div style={{ width:5, height:5, borderRadius:'50%', background:ok?'#22d17a':'#e84040', boxShadow:ok?'0 0 4px #22d17a':'' }}/>
              <span style={{ fontSize:9, fontWeight:700, letterSpacing:'0.07em', color:ok?'#22d17a':'#e84040' }}>{label}</span>
            </div>
          ))}
        </div>
        <div style={{ background:'#2d2d2d', border:'1px solid #3c3c3c', borderRadius:5, padding:'4px 12px', fontSize:11, color:'#7a7a7a', display:'flex', gap:6 }}>
          <span style={{ color:'#f5a623', fontWeight:700 }}>⬡</span><span>Admin</span>
        </div>
      </div>

      {/* Body */}
      <div style={{ flex:1, display:'flex', overflow:'hidden' }}>
        <Sidebar
          projects={projects}
          activeTaskId={activeTaskId}
          activeDeviceId={activeDeviceId}
          onSelectTask={selectTask}
          onNavigate={(taskId, devId) => { openTab(taskId); setActiveDeviceId(devId); }}
          onAddProject={()=>setDlg({type:'newProject'})}
          onAddTask={projId=>setDlg({type:'newTask',projectId:projId})}
          onDeleteTask={deleteTask}
          onAddDevice={taskId=>setDlg({type:'addDevice',taskId})}
          onDeleteDevice={deleteDevice}
          onOpen={()=>{}}
          onSave={()=>{}}
        />

        <div style={{ flex:1, display:'flex', overflow:'hidden' }}>
          {showNavigate ? (
            <NavigateScreen project={activeProject} onSelectTask={selectTask} onAddTask={projId=>setDlg({type:'newTask',projectId:projId})}/>
          ) : openTaskIds.length===0 && !activeProject ? (
            <EmptyHome onNewProject={()=>setDlg({type:'newProject'})} onOpenProject={()=>{}}/>
          ) : (
            <DockWorkspace
              projects={projects}
              allTasks={allTasks}
              openTaskIds={openTaskIds}
              activeTaskId={activeTaskId}
              activeDeviceId={activeDeviceId}
              onSelectTask={id=>{ setActiveTaskId(id); setActiveDeviceId(null); setActiveProjId(null); }}
              onCloseTab={closeTab}
              onOpenTab={openTab}
              setActiveDeviceId={setActiveDeviceId}
              onAddDevice={taskId=>setDlg({type:'addDevice',taskId})}
              onMoveDevice={(taskId,device)=>setDlg({type:'moveDevice',taskId,device})}
              onDeleteDevice={deleteDevice}
              onToggleDeviceConnect={toggleDeviceConnect}
            />
          )}
        </div>
      </div>

      {/* Status bar */}
      <div style={{ height:24, background:'#1563b5', display:'flex', alignItems:'center', padding:'0 12px', gap:16, flexShrink:0 }}>
        <span style={{ fontSize:10, color:'rgba(255,255,255,0.9)', fontWeight:600, display:'flex', alignItems:'center', gap:5 }}>
          <IcoSignal size={11} color="rgba(255,255,255,0.9)"/>
          {allDevices.length} devices · {allDevices.filter(d=>d.connected).length} connected
        </span>
        <span style={{ fontSize:10, color:'rgba(255,255,255,0.5)' }}>|</span>
        <span style={{ fontSize:10, color:'rgba(255,255,255,0.6)' }}>
          {openTaskIds.length} task{openTaskIds.length!==1?'s':''} open
        </span>
        <div style={{ flex:1 }}/>
        <span style={{ fontSize:10, color:'rgba(255,255,255,0.5)', fontFamily:'JetBrains Mono, monospace' }}>NCR Vision Config v2.3.0</span>
      </div>

      {/* Dialogs */}
      {dlg?.type==='newProject' && <NewProjectDialog onConfirm={addProject} onClose={()=>setDlg(null)}/>}
      {dlg?.type==='newTask'    && <NewTaskDialog projectName={getProj(dlg.projectId)?.name||''} onConfirm={addTask} onClose={()=>setDlg(null)}/>}
      {dlg?.type==='addDevice'  && <AddDeviceDialog taskName={getTask(dlg.taskId)?.name||''} otherTasks={allTasks.filter(t=>t.id!==dlg.taskId)} onConfirm={addDevice} onClose={()=>setDlg(null)}/>}
      {dlg?.type==='moveDevice' && <MoveDeviceDialog device={dlg.device} currentTaskId={dlg.taskId} allTasks={allTasks} onConfirm={moveDevice} onClose={()=>setDlg(null)}/>}
    </div>
  );
}

const root = ReactDOM.createRoot(document.getElementById('root'));
root.render(<App/>);
