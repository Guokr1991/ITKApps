/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    GlobalState.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#include "GlobalState.h"


GlobalState
::GlobalState() 
{
  m_GreyFileExtension = NULL;
  m_DrawingColorLabel = 0;
  m_OverWriteColorLabel = 0;
  m_CrosshairsPosition[0] = 0;
  m_CrosshairsPosition[1] = 0;
  m_CrosshairsPosition[2] = 0;
  m_ToolbarMode = CROSSHAIRS_MODE;
  m_CoverageMode = PAINT_OVER_ALL;
  m_UpdateSliceFlag = 1;
  m_InterpolateGrey = false;
  m_InterpolateSegmentation = false;
  m_PolygonInvert = false;
  m_LockHeld = 0;
  m_LockOwner = 0;
  m_SegmentationAlpha = 0;

  // SNAP is off initially
  m_SNAPActive = false;

  // Snake stuff:
  m_SpeedValid = false;
  m_ShowSpeed = false;
  m_IsValidROI = false;
  m_ShowROI = false;
  m_DraggingROI = false;
  m_SnakeMode = EDGE_SNAKE;
  m_SnakeActive = false;

  m_SpeedViewZero = false;
  m_SnakeParameters = SnakeParameters::GetDefaultEdgeParameters();

  // Default preview modes: enabled for in-out, disabled for edges (too slow)
  m_ShowPreprocessedEdgePreview = false;
  m_ShowPreprocessedInOutPreview = true;

  // The preview is not currently valid
  m_SpeedPreviewValid = false;
}

GlobalState
::~GlobalState() 
{
  if(m_GreyFileExtension != NULL)
    delete [] m_GreyFileExtension;
}

#ifdef DRAWING_LOCK

// TODO: What's this for?
// mutual exclusion on the drawing lock - only one window can draw at a time
int 
GlobalState
::GetDrawingLock( short windowID ) 
{
  if ((!m_LockHeld) || (m_LockOwner == windowID)) 
    {
    m_LockHeld = 1;
    m_LockOwner = windowID;
    return 1;
    }
  return 0;
}

int 
GlobalState
::ReleaseDrawingLock(short windowID) 
{
  if (m_LockHeld && (m_LockOwner != windowID)) 
    {
    cerr << "hmph: Cannot ReleaseDrawingLock(): not lock owner" << endl;
    return 0;
    }
  m_LockHeld = 0;
  m_LockOwner = 0;
  return 1;
}

#endif /* DRAWING_LOCK */

/**
 * Pulls the extension off of a filename and
 * saves it for future use in saving preproc
 * data
 *
 * PRE: none
 * POST: if fname is valid, m_GreyFileExtension will
 * have the file extension (including leading
 * period) of the file
 */
void 
GlobalState
::SetGreyExtension(char * fname)
{
  char * basename;
  // Make sure fname is valid
  if(fname != NULL)
    {

    // Get rid of any currently saved
    // extension
    if(m_GreyFileExtension != NULL)
      {                                                 
      delete [] m_GreyFileExtension;
      m_GreyFileExtension = NULL;
      }

    // Get rid of all path information
    // TODO: This is platform specific!!!
    basename = strrchr(fname, '/');
    if(basename)
      {
      basename++;
      }
    else
      {
      basename = fname;
      }

    // Find the first period and count everything
    // to the right of it as path.  Allows for
    // multiple periods (ie .hdr.gz)
    basename = strchr(basename, '.');
    if(basename)
      {
      m_GreyFileExtension = new char[strlen(basename) + 1];
      strcpy(m_GreyFileExtension, basename);
      }
    }
}

/**
 * Returns the extension for the grey file,
 * user is responsible for deallocating the
 * memory for ext
 * PRE: ext is NULL or dynamically allocated array
 * POST: ext points to a null terminated string
 * which contains the extension in m_GreyFileExtension
 * or ext is NULL if m_GreyFileExtension is empty
 */
void 
GlobalState
::GetGreyExtension(char *& ext)
{
  // Get rid of any data in ext
  if(ext != NULL)
    delete [] ext;

  // Copy m_GreyFileExtension into ext
  if(m_GreyFileExtension == NULL)
    ext = NULL;
  else
    {
    ext = new char[strlen(m_GreyFileExtension) + 1];
    strcpy(ext, m_GreyFileExtension);
    }
}
