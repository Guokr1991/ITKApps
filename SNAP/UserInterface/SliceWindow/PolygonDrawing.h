/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    PolygonDrawing.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __PolygonDrawing_h_
#define __PolygonDrawing_h_

#include <FL/Fl.H>
#include "SNAPOpenGL.h"

#include <iostream>
#include <list>
#include "SNAPCommonUI.h"


/**
 * \class PolygonDrawing
 * \brief Code for drawing and editing polygons
 */
class PolygonDrawing
{
public:
  /** States that the polygon drawing is in */
  enum PolygonState { INACTIVE_STATE, DRAWING_STATE, EDITING_STATE };

  /** Vertex structure */
  struct Vertex 
    { 
    float x, y; 
    bool selected;
    Vertex(float x_, float y_, bool on_) : x(x_), y(y_), selected(on_) {}
    Vertex() : x(0.0f), y(0.0f), selected(false) {}
    };

  PolygonDrawing();
  virtual ~PolygonDrawing();
  
  void AcceptPolygon(unsigned char *buffer, int width, int height);
  void PastePolygon(void);
  void Draw(float pixel_x, float pixel_y);
  int  Handle(int event, int button, float x, float y, float pixel_x, float pixel_y);

  void Delete();
  void Insert();
  void Reset();

  /** Get the current state of the polygon editor */
  irisGetMacro(State, PolygonState);
  
  /** How many vertices are selected */
  irisGetMacro(SelectedVertices,bool);

  /** How many vertices are selected */
  irisGetMacro(CachedPolygon,bool);

private:
  // Array of vertices, cached vertices from last operation
  typedef std::list<Vertex> VertexList;
  typedef VertexList::iterator VertexIterator;
  VertexList m_Vertices, m_Cache;

  // State of the system
  PolygonState m_State;

  bool m_CachedPolygon;
  bool m_SelectedVertices;
  bool m_DraggingPickBox;

  // contains selected points
  float m_EditBox[4];         

  // box the user drags to select new points
  float m_SelectionBox[4];         

  float m_StartX, m_StartY;

  GLUtesselatorObj *m_Tesselator;

  void ComputeEditBox();
  void Add(float x, float y, int selected);

  // Callbacks for the GL tesselator
  static void VertexCallback(void *data);
  static void BeginCallback(GLenum which); 
  static void EndCallback(void); 
  static void ErrorCallback(GLenum errorCode);
  static void CombineCallback(
    GLdouble coords[3], GLdouble **irisNotUsed(vertex_data),  
    GLfloat *irisNotUsed(weight), GLdouble **dataOut);
};

#endif // __PolygonDrawing_h_

/*
 *Log: PolygonDrawing.h
 *Revision 1.7  2004/01/27 17:34:00  pauly
 *FIX: Compiling on Mac OSX, issue with GLU include file
 *
 *Revision 1.6  2003/10/09 22:45:15  pauly
 *EMH: Improvements in 3D functionality and snake parameter preview
 *
 *Revision 1.5  2003/10/02 20:57:46  pauly
 *FIX: Made sure that the previous check-in compiles on Linux
 *
 *Revision 1.4  2003/10/02 14:55:53  pauly
 *ENH: Development during the September code freeze
 *
 *Revision 1.1  2003/09/11 13:51:01  pauly
 *FIX: Enabled loading of images with different orientations
 *ENH: Implemented image save and load operations
 *
 *Revision 1.3  2003/08/27 14:03:23  pauly
 *FIX: Made sure that -Wall option in gcc generates 0 warnings.
 *FIX: Removed 'comment within comment' problem in the cvs log.
 *
 *Revision 1.2  2003/08/27 04:57:47  pauly
 *FIX: A large number of bugs has been fixed for 1.4 release
 *
 *Revision 1.1  2003/07/12 04:46:51  pauly
 *Initial checkin of the SNAP application into the InsightApplications tree.
 *
 *Revision 1.4  2003/07/12 01:34:18  pauly
 *More final changes before ITK checkin
 *
 *Revision 1.3  2003/07/11 23:28:10  pauly
 **** empty log message ***
 *
 *Revision 1.2  2003/06/08 23:27:56  pauly
 *Changed variable names using combination of ctags, egrep, and perl.
 *
 *Revision 1.1  2003/03/07 19:29:47  pauly
 *Initial checkin
 *
 *Revision 1.2  2002/12/16 16:40:19  pauly
 **** empty log message ***
 *
 *Revision 1.1.1.1  2002/12/10 01:35:36  pauly
 *Started the project repository
 *
 *
 *Revision 1.2  2002/03/08 14:06:30  moon
 *Added Header and Log tags to all files
 **/
