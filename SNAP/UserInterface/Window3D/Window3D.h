/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    Window3D.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __Window3D_h_
#define __Window3D_h_

#include <FL/Fl.H>
#include <FL/Fl_Gl_Window.H>
#include "SNAPOpenGL.h"
#include <stdlib.h>
#include "FLTKCanvas.h"
#include "SNAPCommonUI.h"
#include "Trackball.h"
#include "MeshObject.h"

// Forward references to parent classes
class UserInterfaceLogic;
class IRISApplication;
class GlobalState;

// Forward references to interactors defined in Window3D.cxx
class Crosshairs3DInteractionMode;
class Trackball3DInteractionMode;
class Spraypaint3DInteractionMode;
class Scalpel3DInteractionMode;

//--------------------------------------------------------------
// plane struct
//-------------------------------------------------------------
struct CutPlaneStruct
{
  // The normal vector in image coordinates
  Vector3d vNormal;

  // The intercept (distance to zero) in image coordinates
  double dIntercept;

  // The normal vector in world coordinates
  Vector3d vNormalWorld;

  // The four corners used to display the plane  
  Vector3d xDisplayCorner[4];

  // The valid/invalid state of the plane
  int valid; 
};

/**
 * \class Window3D
 * \brief Window used to display the 3D segmentation
 */
class Window3D : public FLTKCanvas
{
public:

  enum {X,Y,Z};

  Window3D( int x, int y, int w, int h, const char *l=0 );
  ~Window3D();
  Window3D& operator= ( const Window3D& W ) { return *this; };

  /** Register the parent classes with this application */
  void Register(int i, UserInterfaceLogic *driver);

  /** Initialize the window */
  void Initialize();

  /** Clear the 3D display */
  void ClearScreen();

  /** Reset the trackball to default position */
  void ResetView();

  /** Perform the GL drawing operations */
  void draw();

  /** Recompute the mesh (slow operation) */
  void UpdateMesh();

  /** Respond to user pressing the accept button */
  void Accept();

  /** Enter the cross-hairs mode of operation */
  virtual void EnterCrosshairsMode();
  
  /** Enter the trackball mode of operation */
  virtual void EnterTrackballMode();

  /** Enter the scalpel mode of operation */
  virtual void EnterScalpelMode();
  
  /** Enter the spraypaint mode of operation */
  virtual void EnterSpraypaintMode();

  /** A parent class from which all the Fl event handlers associated
   * with this class should be derived */
  class EventHandler : public InteractionMode {
  public:
    EventHandler(Window3D *parent) {
      m_Parent = parent;
    }    
    void Register() 
    {
      m_Driver = m_Parent->m_Driver;
      m_ParentUI = m_Parent->m_ParentUI;
      m_GlobalState = m_Parent->m_GlobalState;
    }
    virtual int OnKeyDown(const FLTKEvent &e)
      { return m_Parent->OnKeyAction(e.Key); }
  protected:
    Window3D *m_Parent;
    GlobalState *m_GlobalState;
    UserInterfaceLogic *m_ParentUI;
    IRISApplication *m_Driver;
  };

  // Allow friendly access by interactors
  friend class EventHandler;
  friend class Trackball3DInteractionMode;
  friend class Crosshairs3DInteractionMode;
  friend class Scalpel3DInteractionMode;
  friend class Spraypaint3DInteractionMode;

  // Methods called by these interactors (unlike GenericSliceWindow, this window
  // does not delegate much of its control to the interactors and only provides
  // these interfaces for them
  void OnRotateStartAction(int x, int y);
  void OnPanStartAction(int x, int y);
  void OnZoomStartAction(int x, int y);
  void OnTrackballDragAction(int x, int y);
  void OnTrackballStopAction();
  void OnCrosshairClickAction(int x,int y);
  void OnSpraypaintClickAction(int x,int y);
  void OnScalpelPointPairAction(int x1, int y1, int x2, int y2);
  int OnKeyAction(int key);

private:

  // Pointer to the application driver for this UI object
  IRISApplication *m_Driver;

  // Pointer to the global state object (shorthand)
  GlobalState *m_GlobalState;

  // Pointer to GUI that contains this Window3D object
  UserInterfaceLogic *m_ParentUI;   

  // Interaction modes
  Crosshairs3DInteractionMode *m_CrosshairsMode;
  Trackball3DInteractionMode *m_TrackballMode;
  Spraypaint3DInteractionMode *m_SpraypaintMode;
  Scalpel3DInteractionMode *m_ScalpelMode;

  // Cut planes 3d mode added by Robin
  enum Win3DMode
    {
    WIN3D_NONE, WIN3D_PAN, WIN3D_ZOOM, WIN3D_ROTATE, WIN3D_CUT
    };


  Win3DMode m_Mode;
  Trackball  m_Trackball, m_TrackballBackup;
  MeshObject m_Mesh;
  CutPlaneStruct m_Plane;

  // id of window
  int m_Id;
  int m_NeedsInitialization;
  int m_CursorVisible;

  // Coordinate systems: image coords have unit distance equal to one voxel,
  // origin is at the origin of the image (left bottom back).
  // World coords have same origin, but distance is in mm.
  // The mesh object is in world coords, but the crosshairs position and the
  // rays to fire into the seg data are in image coords.

  // Extent of the image, in image coords
  Vector3ui m_ImageSize;   

  // Dimensions of a voxel, in mm
  Vector3f m_Spacing;   

  // Dimensions of the image (dimensions * spacing)
  Vector3f m_VolumeSize;

  // Center of image, in world coords
  Vector3f m_Center;    

  // Center of rotation, in world coords
  Vector3f m_CenterOfRotation;    

  // width of view, in world coords
  Vector3f m_DefaultHalf, m_ViewHalf; 

  // A ray and a point used for projecting mouse-clicks onto the 3D surface
  Vector3d m_Ray;
  Vector3d m_Point;

  // An array of samples
  typedef std::list<Vector3i> SampleListType;
  typedef SampleListType::iterator SampleListIterator;
  SampleListType m_Samples;

  void MousePressFunc(int button);
  void MouseReleaseFunc();
  void MouseMotionFunc();
  void MouseCrossPressFunc(int button);
  void MousePointPressFunc(int button);
  void MouseCutPressFunc(int button);
  void SetupModelView();
  void SetupProjection();
  void ComputeMatricies( GLint *vport, double *mview, double *proj );
  void ComputeRay( int x, int y, double *mvmatrix, double *projmat,
                   GLint *viewport, Vector3d &v, Vector3d &r );

  /** Try to compute cut plane.  Return false if the points coincide */
  bool ComputeCutPlane(int wx1, int wy1, int wx2, int wy2);

  int IntersectSegData(int mouse_x, int mouse_y, Vector3i &hit);

  // void OnCutPlanePointRayAction(int mouse_x, int mouse_y, int i);
  // void ComputePlane();
  void DrawCutPlane(); // Added by Robin & Ming

  void AddSample( Vector3i s );
  void DrawCrosshairs();
  void DrawSamples();

  void CheckErrors();

};

#endif // __Window3D_h_

/*
 *Log: Window3D.h
 *Revision 1.7  2004/01/27 18:05:38  pauly
 *FIX: More MAC OSX fixes. Also removed old snake code no longer in use
 *
 *Revision 1.6  2004/01/27 17:34:00  pauly
 *FIX: Compiling on Mac OSX, issue with GLU include file
 *
 *Revision 1.5  2003/10/09 22:45:15  pauly
 *EMH: Improvements in 3D functionality and snake parameter preview
 *
 *Revision 1.4  2003/10/02 14:55:53  pauly
 *ENH: Development during the September code freeze
 *
 *Revision 1.1  2003/09/11 13:51:15  pauly
 *FIX: Enabled loading of images with different orientations
 *ENH: Implemented image save and load operations
 *
 *Revision 1.3  2003/08/27 14:03:24  pauly
 *FIX: Made sure that -Wall option in gcc generates 0 warnings.
 *FIX: Removed 'comment within comment' problem in the cvs log.
 *
 *Revision 1.2  2003/08/27 04:57:47  pauly
 *FIX: A large number of bugs has been fixed for 1.4 release
 *
 *Revision 1.1  2003/07/12 04:46:51  pauly
 *Initial checkin of the SNAP application into the InsightApplications tree.
 *
 *Revision 1.6  2003/07/12 01:34:18  pauly
 *More final changes before ITK checkin
 *
 *Revision 1.5  2003/07/11 23:25:33  pauly
 **** empty log message ***
 *
 *Revision 1.4  2003/06/08 23:27:56  pauly
 *Changed variable names using combination of ctags, egrep, and perl.
 *
 *Revision 1.3  2003/04/23 06:05:18  pauly
 **** empty log message ***
 *
 *Revision 1.2  2003/04/16 05:04:17  pauly
 *Incorporated intensity modification into the snap pipeline
 *New IRISApplication
 *Random goodies
 *
 *Revision 1.1  2003/03/07 19:29:48  pauly
 *Initial checkin
 *
 *Revision 1.1.1.1  2002/12/10 01:35:36  pauly
 *Started the project repository
 *
 *
 *Revision 1.2  2002/03/08 14:06:32  moon
 *Added Header and Log tags to all files
 **/
