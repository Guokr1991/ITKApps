/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    PolygonDrawing.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#include "PolygonDrawing.h"
#include "SNAPCommonUI.h"

#include "SNAPOpenGL.h"
#include <iostream>
#include <cstdlib>
#include <algorithm>

using namespace std;

// glu Tess callbacks

/*
#ifdef WIN32
typedef void (CALLBACK *TessCallback)();
#else
typedef void (*TessCallback)();
#endif

void 
#ifdef WIN32
CALLBACK 
#endif
BeginCallback(GLenum which)
{
  glBegin(which);
}

void 
#ifdef WIN32
CALLBACK 
#endif
EndCallback(void) 
{
  glEnd();
}

void 
#ifdef WIN32
CALLBACK
#endif
ErrorCallback(GLenum errorCode)
{
  const GLubyte *estring;

  estring = gluErrorString(errorCode);
  cerr << "Tesselation Error-Exiting: " << estring << endl;
  exit(-1);
}


void 
#ifdef WIN32
CALLBACK 
#endif
CombineCallback(GLdouble coords[3], 
                GLdouble **irisNotUsed(vertex_data),  
                GLfloat *irisNotUsed(weight), 
                GLdouble **dataOut) 
{
  GLdouble *vertex;

  vertex = new GLdouble[3];
  vertex[0] = coords[0];
  vertex[1] = coords[1];
  vertex[2] = coords[2];
  *dataOut = vertex;
}
*/

/**
 * PolygonDrawing()
 *
 * purpose: 
 * create initial vertex and m_Cache arrays, init GLUm_Tesselatorelator
 */
PolygonDrawing
::PolygonDrawing()
{
  m_CachedPolygon = false;
  m_State = INACTIVE_STATE;
  m_SelectedVertices = false;
  m_DraggingPickBox = false;

  // Typecast for the callback functions
  #ifdef WIN32
  typedef void (CALLBACK *TessCallback)();
  #else
  typedef void (*TessCallback)();
  #endif

  // create glu Tesselator for rendering polygons
  m_Tesselator = gluNewTess();
  gluTessCallback(m_Tesselator,(GLenum) GLU_TESS_VERTEX, (TessCallback) glVertex3dv);
  gluTessCallback(m_Tesselator,(GLenum) GLU_TESS_BEGIN, (TessCallback) glBegin); 
  gluTessCallback(m_Tesselator,(GLenum) GLU_TESS_END, (TessCallback) glEnd);
  gluTessCallback(m_Tesselator,(GLenum) GLU_TESS_ERROR, 
    (TessCallback) &PolygonDrawing::ErrorCallback);     
  gluTessCallback(m_Tesselator,(GLenum) GLU_TESS_COMBINE, 
    (TessCallback) &PolygonDrawing::CombineCallback);
  gluTessProperty(m_Tesselator,(GLenum) GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_NONZERO);  
  gluTessNormal(m_Tesselator,0.0,0.0,1.0);
}

/**
 * ~PolygonDrawing()
 *
 * purpose: 
 * free arrays and GLUm_Tesselatorelator
 */
PolygonDrawing
::~PolygonDrawing()
{
  gluDeleteTess(m_Tesselator);
}

/**
 * ComputeEditBox()
 *
 * purpose: 
 * compute the bounding box around selected vertices
 * 
 * post:
 * if m_Vertices are selected, sets m_SelectedVertices to 1, else 0
 */
void
PolygonDrawing
::ComputeEditBox() 
{
  VertexIterator it;

  // Find the first selected vertex and initialize the selection box
  m_SelectedVertices = false;
  for (it = m_Vertices.begin(); it!=m_Vertices.end();++it) 
    {
    if (it->selected) 
      {
      m_EditBox[0] = m_EditBox[1] = it->x;
      m_EditBox[2] = m_EditBox[3] = it->y;
      m_SelectedVertices = true;
      break;
      }
    }

  // Continue only if a selection exists
  if (!m_SelectedVertices) return;

  // Grow selection box to fit all selected vertices
  for(it = m_Vertices.begin(); it!=m_Vertices.end();++it)
    {
    if (it->selected) 
      {
      if (it->x < m_EditBox[0]) m_EditBox[0] = it->x;
      else if (it->x > m_EditBox[1]) m_EditBox[1] = it->x;

      if (it->y < m_EditBox[2]) m_EditBox[2] = it->y;
      else if (it->y > m_EditBox[3]) m_EditBox[3] = it->y;
      }
    }
}

/**
 * Add()
 *
 * purpose:
 * to add a vertex to the existing contour
 * 
 * pre: 
 * m_NumberOfAllocatedVertices > 0
 */
/*
void
PolygonDrawing
::Add(float x, float y, int selected)
{
  // add a new vertex
  Vertex vNew;
  vNew.x = x; vNew.y = y; vNew.selected = selected;


  m_Vertices[m_NumberOfUsedVertices].x = x;
  m_Vertices[m_NumberOfUsedVertices].y = y;
  m_Vertices[m_NumberOfUsedVertices].selected = selected;
    
  m_NumberOfUsedVertices++;
}
*/

/**
 * Delete()
 *
 * purpose: 
 * delete all vertices that are selected
 * 
 * post: 
 * if all m_Vertices removed, m_State becomes INACTIVE_STATE
 * length of m_Vertices array does not decrease
 */
void
PolygonDrawing
::Delete() 
{
  VertexIterator it=m_Vertices.begin();
  while(it!=m_Vertices.end())
    {
    if(it->selected)
      it = m_Vertices.erase(it);
    else ++it;
    }
  
  if (m_Vertices.empty()) 
    m_State = INACTIVE_STATE;
}

void 
PolygonDrawing
::Reset()
{
  m_State = INACTIVE_STATE;
  m_Vertices.clear();
}

/**
 * Insert()
 *
 * purpose:
 * insert vertices between adjacent selected vertices
 * 
 * post: 
 * length of m_Vertices array does not decrease
 */
void
PolygonDrawing
::Insert() 
{
  // Insert a vertex between every pair of adjacent vertices
  VertexIterator it = m_Vertices.begin();
  while(it != m_Vertices.end())
    {
    // Get the itNext iterator to point to the next point in the list
    VertexIterator itNext = it;
    if(++itNext == m_Vertices.end()) 
      itNext = m_Vertices.begin();

    // Check if the insertion is needed
    if(it->selected && itNext->selected)
      {
      // Insert a new vertex
      Vertex vNew(0.5 * (it->x + itNext->x), 0.5 * (it->y + itNext->y), true);
      it = m_Vertices.insert(++it, vNew);
      }

    // On to the next point
    ++it;
    }
}

/**
 * AcceptPolygon()
 *
 * purpose:
 * to rasterize the current polygon into a buffer & copy the edited polygon
 * into the polygon m_Cache
 *
 * parameters:
 * buffer - an array of unsigned chars interpreted as an RGBA buffer
 * width  - the width of the buffer
 * height - the height of the buffer 
 *
 * pre: 
 * buffer array has size width*height*4
 * m_State == EDITING_STATE
 *
 * post: 
 * m_State == INACTIVE_STATE
 */
void 
PolygonDrawing
::AcceptPolygon(unsigned char *buffer, int width, int height) 
{
  // Push the GL attributes
  glPushAttrib(GL_ALL_ATTRIB_BITS);

  // Tesselate the polygon and save the tesselation as a display list
  GLint dl = glGenLists(1);
  glNewList(dl, GL_COMPILE);

  // Set the background to black
  glClearColor(0,0,0,1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Paint in white
  glColor3d(1.0, 1.0, 1.0);

  // Allocate an array to hold the vertices
  double *vArray = new double[3 * m_Vertices.size() + 3], *vPointer = vArray;
  for(VertexIterator it = m_Vertices.begin(); it!=m_Vertices.end();++it)
    {
    *vPointer++ = (double) it->x;
    *vPointer++ = (double) it->y;
    *vPointer++ = 0.0;
    }

  // Start the tesselation
  gluTessBeginPolygon(m_Tesselator,NULL);
  gluTessBeginContour(m_Tesselator);

  // Add the vertices
  for(unsigned int i=0; i < m_Vertices.size(); i++)
    { gluTessVertex(m_Tesselator, vArray + 3*i, vArray + 3*i); }
    
  // End the tesselation
  gluTessEndContour(m_Tesselator);
  gluTessEndPolygon(m_Tesselator);

  // Clean up the array
  delete vArray;

  // End the display list
  glEndList();

  // Draw polygon into back buffer - back buffer should get redrawn
  // anyway before it gets swapped to the screen.
  glDrawBuffer(GL_BACK);
  glReadBuffer(GL_BACK);

  // We will perform a tiled drawing, because the backbuffer may be smaller
  // than the size of the image. First get the viewport size, i.e., tile size
  GLint xViewport[4]; glGetIntegerv(GL_VIEWPORT, xViewport);
  unsigned int wTile = (unsigned int) xViewport[2];
  unsigned int hTile = (unsigned int) xViewport[3];

  // Figure out the number of tiles in x and y dimension
  unsigned int nTilesX = (unsigned int) ceil( width * 1.0 / wTile );
  unsigned int nTilesY = (unsigned int) ceil( height * 1.0 / hTile );

  // Draw and retrieve each tile
  for(int iTileX = 0; iTileX < nTilesX; iTileX++)
    {
    for(int iTileY = 0; iTileY < nTilesY; iTileY++)
      {
      // Get the corner of the tile
      unsigned int xTile = iTileX * wTile, yTile = iTileY * hTile;

      // Set the projection matrix
      glMatrixMode(GL_PROJECTION);
      glPushMatrix();
      glLoadIdentity();
      gluOrtho2D(xTile, xTile + wTile, yTile, yTile + hTile);

      // Set the model view matrix
      glMatrixMode(GL_MODELVIEW);
      glPushMatrix();
      glLoadIdentity();

      // Draw the triangles
      glCallList(dl);

      // Figure out the size of the data chunk to copy
      unsigned int wCopy = width - xTile < wTile ? width - xTile : wTile;
      unsigned int hCopy = height - yTile < hTile ? height - yTile : hTile;
      
      // Set up the copy so that the strides are correct
      glPixelStorei(GL_PACK_ALIGNMENT, 1);
      glPixelStorei(GL_PACK_ROW_LENGTH, width);
      glPixelStorei(GL_PACK_SKIP_PIXELS, xTile);
      glPixelStorei(GL_PACK_SKIP_ROWS, yTile);

      // Copy the pixels to the buffer
      glReadPixels(0,0,wCopy,hCopy,GL_LUMINANCE,GL_UNSIGNED_BYTE,buffer);

      // Restore the GL state
      glPopMatrix();
      glMatrixMode(GL_PROJECTION);
      glPopMatrix();
      }
    }

  // Get rid of the display list
  glDeleteLists(dl,1);

  // Restore the GL state
  glPopAttrib();

  // Copy polygon into polygon m_Cache
  m_CachedPolygon = true;
  m_Cache = m_Vertices;

  // Reset the vertex array for next time
  m_Vertices.clear();
  m_SelectedVertices = false;

  // Set the state
  m_State = INACTIVE_STATE;
}

/**
 * PastePolygon()
 *
 * purpose:
 * copy the m_Cached polygon to the edited polygon
 * 
 * pre: 
 * m_CachedPolygon == 1
 * m_State == INACTIVE_STATE
 * 
 * post: 
 * m_State == EDITING_STATE
 */
void 
PolygonDrawing
::PastePolygon(void)
{
  // Copy the cache into the vertices
  m_Vertices = m_Cache;

  // Select everything
  for(VertexIterator it = m_Vertices.begin(); it!=m_Vertices.end();++it)
    it->selected = true;

  // Set the state
  m_SelectedVertices = true;
  m_State = EDITING_STATE;
  
  // Compute the edit box
  ComputeEditBox();
}

/**
 * Draw()
 *
 * purpose: 
 * draw the polyline being drawn or the polygon being edited
 *
 * parameters:
 * pixel_x - this is a width in the polygon's space that is a single 
 *           pixel width on screen
 * pixel_y - this is a height in the polygon's space that is a single 
 *           pixel height on screen
 *  
 * pre: none - expected to exit if m_State is INACTIVE_STATE
 */
void
PolygonDrawing
::Draw(float pixel_x, float pixel_y)
{
  // Must be in active state
  if (m_State == INACTIVE_STATE) return;

  // Push the line state
  glPushAttrib(GL_LINE_BIT | GL_COLOR_BUFFER_BIT);  

  // set line and point drawing parameters
  glPointSize(4);
  glLineWidth(2);
  glEnable(GL_LINE_SMOOTH);
  glHint(GL_LINE_SMOOTH_HINT,GL_NICEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Draw the line segments
  VertexIterator it, itNext;
  if (m_State == EDITING_STATE)
  {
    glBegin(GL_LINES);
    for(it = m_Vertices.begin(); it!=m_Vertices.end();++it)
      {
      // Point to the next vertex
      itNext = it; ++itNext; 
      if(itNext == m_Vertices.end())
        itNext = m_Vertices.begin();

      // Set the color based on the mode
      if (it->selected && itNext->selected) 
        glColor3f(0,1,0);
      else 
        glColor3f(1,0,0);
  
      // Draw the line
      glVertex3f(it->x, it->y,0);
      glVertex3f(itNext->x, itNext->y, 0);
    }
    glEnd();
  }
  else 
  {
    glBegin(GL_LINE_STRIP);
    glColor3f(1,0,0);
    for(it = m_Vertices.begin(); it!=m_Vertices.end();++it) 
      glVertex3f(it->x, it->y, 0);
    glEnd();
  }
    
  // draw the vertices
  glBegin(GL_POINTS);
  for(it = m_Vertices.begin(); it!=m_Vertices.end();++it) 
  {
    if (it->selected) 
      glColor3f(0.0f, 1.0f, 0.0f); 
    else
      glColor3f(1.0f, 0.0f, 0.0f);

    glVertex3f(it->x,it->y,0.0f);
  }
  glEnd();

  // draw edit or pick box
  if (m_DraggingPickBox) 
  {
    glLineWidth(1);
    glColor3f(0, 1, 0);
    glBegin(GL_LINE_LOOP);
    glVertex3f(m_SelectionBox[0],m_SelectionBox[2],0.0);
    glVertex3f(m_SelectionBox[1],m_SelectionBox[2],0.0);
    glVertex3f(m_SelectionBox[1],m_SelectionBox[3],0.0);
    glVertex3f(m_SelectionBox[0],m_SelectionBox[3],0.0);
    glEnd();    
  }
  else if (m_SelectedVertices) 
  {
    float border_x = (float)4.0 * pixel_x;
    float border_y = (float)4.0 * pixel_y;
    glLineWidth(1);
    glColor3f(0, 1, 0);
    glBegin(GL_LINE_LOOP);
    glVertex3f(m_EditBox[0] - border_x,m_EditBox[2] - border_y,0.0);
    glVertex3f(m_EditBox[1] + border_x,m_EditBox[2] - border_y,0.0);
    glVertex3f(m_EditBox[1] + border_x,m_EditBox[3] + border_y,0.0);
    glVertex3f(m_EditBox[0] - border_x,m_EditBox[3] + border_y,0.0);
    glEnd();
  }

  glPopAttrib();
}

/**
 * Handle()
 *
 * purpose:
 * handle events from the window that contains polygon drawing object:
 * if internal m_State is DRAWING_STATE
 *   left-click creates subsequent vertices of the polygon, 
 *   right-click closes polygon and puts polygon in EDITING_STATE
 * if internal m_State is EDITING_STATE
 *   shift-left-click on vertex adds vertex to selection
 *   shift-left-click off vertex begins dragging a rubber-band box
 *
 *   shift-right-click performs same actions but de-selects vertices
 *
 *   left-click in selection box begins dragging of selected vertices
 *   left-click outside selection box cancels selection, begins a 
 *   dragging of a rubber-band box
 *   
 *   left-release after dragging adds vertices inside rubber-band box
 *   to selection
 * 
 *   pressing the 'insert' key calls Insert()
 *   pressing the 'delete' key calls Delete()
 * 
 * parameters:
 * event   - an Fl event number
 * x       - the x of the point clicked in the space of the polygon
 * y       - the y of the point clicked in the space of the polygon
 * pixel_x - see Draw()
 * pixel_y - see Draw()
 *
 * pre: 
 * window that calls this has the drawing lock
 * 
 * post:
 * if event is used, 1 is returned, else 0
 */
int
PolygonDrawing
::Handle(int event, int button, float x, float y, 
         float pixel_x, float pixel_y)
{
  VertexIterator it, itNext;

  switch (m_State) {
  case INACTIVE_STATE:
    if ((event == FL_PUSH) && (button == FL_LEFT_MOUSE)) 
      {
      m_State = DRAWING_STATE;
      m_Vertices.push_back( Vertex(x, y, false) );
      return 1;
      }
    break;

  case DRAWING_STATE:
    if (event == FL_PUSH) 
      {
      if (button == FL_LEFT_MOUSE)
        {
        m_Vertices.push_back( Vertex(x, y, false) );
        return 1;
        } 
      else if (button == FL_RIGHT_MOUSE) 
        {
        m_State = EDITING_STATE;
        m_SelectedVertices = true;

        for(it = m_Vertices.begin(); it!=m_Vertices.end(); ++it)
          it->selected = true;

        ComputeEditBox();
        return 1;
        }
      }
    break;

  case EDITING_STATE:
    switch (event) {
    case FL_PUSH:
      m_StartX = x;
      m_StartY = y;

      if ((button == FL_LEFT_MOUSE) || (button == FL_RIGHT_MOUSE)) 
        {

        // if user is pressing shift key, add/toggle m_Vertices, or drag pick box
        if (Fl::event_state(FL_SHIFT)) 
          {
          // check if vertex clicked
          for(it = m_Vertices.begin(); it!=m_Vertices.end(); ++it)
            {
            if((x >= it->x - 2.0*pixel_x) && (x <= it->x + 2.0*pixel_x)
              && (y >= it->y - 2.0*pixel_y) && (y <= it->y + 2.0*pixel_y)) 
              {
              it->selected = (button == 1);
              ComputeEditBox();
              return 1;
              }
            }

          // otherwise start dragging pick box
          m_DraggingPickBox = true;
          m_SelectionBox[0] = m_SelectionBox[1] = x;
          m_SelectionBox[2] = m_SelectionBox[3] = y;
          return 1;
          }

        // user not holding shift key; if user clicked inside edit box, 
        // edit box will be moved in drag event
        if (m_SelectedVertices &&
          (x >= (m_EditBox[0] - 4.0*pixel_x)) && 
          (x <= (m_EditBox[1] + 4.0*pixel_x)) && 
          (y >= (m_EditBox[2] - 4.0*pixel_y)) && 
          (y <= (m_EditBox[3] + 4.0*pixel_y))) return 1;

        // clicked outside of edit box & shift not held, this means the 
        // current selection will be cleared
        for(it = m_Vertices.begin(); it!=m_Vertices.end(); ++it) 
          it->selected = false;
        m_SelectedVertices = false;

        // check if point clicked
        for(it = m_Vertices.begin(); it!=m_Vertices.end(); ++it)  
          {
          if((x >= it->x - 2.0*pixel_x) &&(x <= it->x + 2.0*pixel_x) 
            && (y >= it->y - 2.0*pixel_y) && (y <= it->y + 2.0*pixel_y)) 
            {
            it->selected = true;
            ComputeEditBox();
            return 1;
            }
          }

        // didn't click a point - start dragging pick box
        m_DraggingPickBox = true;
        m_SelectionBox[0] = m_SelectionBox[1] = x;
        m_SelectionBox[2] = m_SelectionBox[3] = y;
        return 1;
        }
      break;

    case FL_DRAG:
      if ((button == FL_LEFT_MOUSE) || (button == FL_RIGHT_MOUSE)) 
        {
        if (m_DraggingPickBox) 
          {
          m_SelectionBox[1] = x;
          m_SelectionBox[3] = y;
          } 
        else 
          {
          if (button == FL_LEFT_MOUSE) 
            {
            m_EditBox[0] += (x - m_StartX);
            m_EditBox[1] += (x - m_StartX);
            m_EditBox[2] += (y - m_StartY);
            m_EditBox[3] += (y - m_StartY);

            for(it = m_Vertices.begin(); it!=m_Vertices.end(); ++it) 
              {
              if (it->selected) 
                {
                it->x += (x - m_StartX);
                it->y += (y - m_StartY);
                }
              }
            m_StartX = x;
            m_StartY = y;
            }
          }
        return 1;
        }
      break;

    case FL_RELEASE:
      if ((button == FL_LEFT_MOUSE) || (button == FL_RIGHT_MOUSE)) 
        {
        if (m_DraggingPickBox) 
          {
          m_DraggingPickBox = false;

          float temp;
          if (m_SelectionBox[0] > m_SelectionBox[1]) 
            {
            temp = m_SelectionBox[0];
            m_SelectionBox[0] = m_SelectionBox[1];
            m_SelectionBox[1] = temp;
            }
          if (m_SelectionBox[2] > m_SelectionBox[3]) 
            {
            temp = m_SelectionBox[2];
            m_SelectionBox[2] = m_SelectionBox[3];
            m_SelectionBox[3] = temp;
            }

          for(it = m_Vertices.begin(); it!=m_Vertices.end(); ++it) 
            {
            if((it->x >= m_SelectionBox[0]) && (it->x <= m_SelectionBox[1]) 
              && (it->y >= m_SelectionBox[2]) && (it->y <= m_SelectionBox[3]))
              it->selected = (button == 1);
            }
          ComputeEditBox();
          }
        return 1;
        }
      break;

    case FL_SHORTCUT:
      if (Fl::event_key(FL_Delete) || Fl::event_key(FL_BackSpace) ||
        (Fl::event_state(FL_CTRL) && Fl::event_key('d')) ||
        (Fl::event_state(FL_CTRL) && Fl::event_key('D')) ) 
        { 
        Delete(); 
        return 1;
        } 
      else if (Fl::event_key(FL_Insert) ||
        (Fl::event_state(FL_CTRL) && Fl::event_key('i')) ||
        (Fl::event_state(FL_CTRL) && Fl::event_key('I')) ) 
        {
        Insert(); 
        return 1;
        }
      break;

    default: break;
    }
    break;

  default: cerr << "PolygonDrawing::Handle(): unknown m_State " << m_State << endl;
  }

  return 0;
}

void PolygonDrawing::VertexCallback(void *data)
{
  glVertex3dv((double *)data);
}

void PolygonDrawing::BeginCallback(GLenum which) 
{ 
  glBegin(which); 
}

void PolygonDrawing::EndCallback(void) 
{ 
  glEnd(); 
}

void PolygonDrawing::ErrorCallback(GLenum errorCode)
{ 
  cerr << "Tesselation Error: " << gluErrorString(errorCode) << endl; 
}

void PolygonDrawing::CombineCallback(GLdouble coords[3], 
                GLdouble **irisNotUsed(vertex_data),  
                GLfloat *irisNotUsed(weight), 
                GLdouble **dataOut) 
{
  GLdouble *vertex = new GLdouble[3];
  vertex[0] = coords[0]; vertex[1] = coords[1]; vertex[2] = coords[2];
  *dataOut = vertex;
}


/*
 *Log: PolygonDrawing.cxx
 *Revision 1.7  2004/01/27 17:49:47  pauly
 *FIX: MAC OSX Compilation fixes
 *
 *Revision 1.6  2003/10/09 22:45:15  pauly
 *EMH: Improvements in 3D functionality and snake parameter preview
 *
 *Revision 1.5  2003/10/02 14:55:53  pauly
 *ENH: Development during the September code freeze
 *
 *Revision 1.1  2003/09/11 13:51:01  pauly
 *FIX: Enabled loading of images with different orientations
 *ENH: Implemented image save and load operations
 *
 *Revision 1.4  2003/08/28 14:37:09  pauly
 *FIX: Clean 'unused parameter' and 'static keyword' warnings in gcc.
 *FIX: Label editor repaired
 *
 *Revision 1.3  2003/08/27 14:03:23  pauly
 *FIX: Made sure that -Wall option in gcc generates 0 warnings.
 *FIX: Removed 'comment within comment' problem in the cvs log.
 *
 *Revision 1.2  2003/08/27 04:57:47  pauly
 *FIX: A large number of bugs has been fixed for 1.4 release
 *
 *Revision 1.1  2003/07/12 04:46:50  pauly
 *Initial checkin of the SNAP application into the InsightApplications tree.
 *
 *Revision 1.1  2003/07/11 23:28:10  pauly
 **** empty log message ***
 *
 *Revision 1.3  2003/06/08 23:27:56  pauly
 *Changed variable names using combination of ctags, egrep, and perl.
 *
 *Revision 1.2  2003/04/29 14:01:42  pauly
 *Charlotte Trip
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
