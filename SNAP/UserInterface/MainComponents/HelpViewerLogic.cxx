/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    HelpViewerLogic.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#include "HelpViewerLogic.h"

using namespace std;

// Callback for when links are clicked
const char *HelpViewerLogicLinkCallback(Fl_Widget *w, const char *uri)
{
  HelpViewerLogic *hvl = static_cast<HelpViewerLogic *>(w->user_data());
  return hvl->LinkCallback(uri);  
}

HelpViewerLogic
::HelpViewerLogic()
{
  m_Iterator = m_LinkList.begin();  
}
  
HelpViewerLogic
::~HelpViewerLogic()
{
}

void 
HelpViewerLogic
::ShowHelp(const char *url)
{
  // Put the url into the history list
  PushURL(url);
  
  // Configure the browser
  m_BrsHelp->link(HelpViewerLogicLinkCallback);
  m_BrsHelp->user_data(this);
  m_BrsHelp->load(url); 

  // Show the window
  if(!m_WinHelp->shown())
    m_WinHelp->show();
}

const char *
HelpViewerLogic
::LinkCallback(const char *url)
{
  // Put the url into the history list
  PushURL(url);

  // Return the URL unchanged
  return url;
}

void 
HelpViewerLogic
::PushURL(const char *url)
{
  // If the link is the same as our current location, don't do anything
  if(m_Iterator != m_LinkList.end() && 
     0 == strcmp(m_Iterator->c_str(),url))
    return;

  // Clear the forward stack
  m_LinkList.erase(++m_Iterator,m_LinkList.end());

  // Add the new link to the list
  m_LinkList.push_back(string(url));

  // Point to the end of the list
  m_Iterator = m_LinkList.end();
  m_Iterator--;

  // Disable the forward button
  m_BtnForward->deactivate();

  // Activate the back button if there is a back link
  if(m_Iterator != m_LinkList.begin())
    m_BtnBack->activate();
  else
    m_BtnBack->deactivate();    
}

void 
HelpViewerLogic
::OnLinkAction()
{
}

void 
HelpViewerLogic
::OnBackAction()
{
  // Can't be at the head of the list
  assert(m_Iterator != m_LinkList.begin());
  
  // Go back with the iterator
  --m_Iterator;

  // Enable the forward button
  m_BtnForward->activate();

  // Perhaps disable the back button
  if(m_Iterator == m_LinkList.begin())
    m_BtnBack->deactivate();

  // Show the current link
  m_BrsHelp->load(m_Iterator->c_str());
}

void 
HelpViewerLogic
::OnForwardAction()
{
  // Can't be at the end of the list
  assert(++m_Iterator != m_LinkList.end());

  // Enable the back button
  m_BtnBack->activate();

  // Perhaps disable the forward button
  LinkIterator itTest = m_Iterator;
  if(++itTest == m_LinkList.end())
    m_BtnForward->deactivate();

  // Show the current link
  m_BrsHelp->load(m_Iterator->c_str());
}

void 
HelpViewerLogic
::OnCloseAction()
{
  m_WinHelp->hide();
}

void 
HelpViewerLogic
::OnContentsAction()
{
  // Go to the contents page
  ShowHelp(m_ContentsLink.c_str());  
}


