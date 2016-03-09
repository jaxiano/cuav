/*!
 * \brief workerthread class declaration
 */
class WorkerThread : public QThread
{
  Q_OBJECT
public:
  WorkerThread(Mainview *mainWindow)
    : mainWindow(mainWindow){};
  virtual ~WorkerThread(){};

  void run() {mainWindow->run();}
protected:
  Mainview *mainWindow;
};


[initialization]
Mainview::Mainview()
{
    // init worker Thread
    m_workThread = NULL;
    prepareWorkThread();
}

bool Mainview::prepareWorkThread()
{
  if(!m_workThread) m_workThread = new WorkerThread(this);
  if(m_workThread->isRunning()) return false; // Not
  return true;
}



[open camera]
    // if start ID is given with commandline then start
    // worker thread with initialzing the camera
    if (strStartID != "")
    {
        // get the commandline start ID
        unsigned int camid = strStartID.toInt();

        // init the cameralist for getting device infos
        pDlgCamerList = new CameraList (this);
        if (pDlgCamerList->SelectCameraByID(camid)==0)
        {
            m_CamListInfo = pDlgCamerList->GetCameraInfo ();

            // initialize and start the worker thread
            m_eWorkthreadJob = wj_cameraopenlive;
            m_workthreadReturn = IS_SUCCESS;
            m_workThread->start();
        }
        delete pDlgCamerList;
        pDlgCamerList = 0;
    }

void Mainview::run()
{
  switch(m_eWorkthreadJob)
  {
  case wj_cameraopen:
  case wj_cameraopenlive:
      // open the camera
      m_hCamera = (HIDS) (m_CamListInfo.dwDeviceID | IS_USE_DEVICE_ID); // open next camera
      if (m_bAutomaticSEStarterUpload)
      {
          m_hCamera |= IS_ALLOW_STARTER_FW_UPLOAD;
      }
      m_workthreadReturn = is_InitCamera (&m_hCamera, 0);
      break;
  default:
      break;
  }
}


[load parameters]
            ret = is_ParameterSet( m_hCamera, IS_PARAMETERSET_CMD_LOAD_EEPROM, NULL, 0 );

[
