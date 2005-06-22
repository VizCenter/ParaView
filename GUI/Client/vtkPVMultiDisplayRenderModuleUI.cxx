/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMultiDisplayRenderModuleUI.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVMultiDisplayRenderModuleUI.h"
#include "vtkObjectFactory.h"
#include "vtkKWCheckButton.h"



//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVMultiDisplayRenderModuleUI);
vtkCxxRevisionMacro(vtkPVMultiDisplayRenderModuleUI, "1.9");

//----------------------------------------------------------------------------
vtkPVMultiDisplayRenderModuleUI::vtkPVMultiDisplayRenderModuleUI()
{
  this->CompositeOptionEnabled = 0;
}

//----------------------------------------------------------------------------
vtkPVMultiDisplayRenderModuleUI::~vtkPVMultiDisplayRenderModuleUI()
{
}

//----------------------------------------------------------------------------
void vtkPVMultiDisplayRenderModuleUI::Create(vtkKWApplication *app)
{
  if (this->IsCreated())
    {
    vtkErrorMacro("vtkPVMultiDisplayRenderModuleUI already created");
    return;
    }

  this->Superclass::Create(app);

  // We do not have these options.
  this->CompositeWithFloatCheck->SetState(0);
  this->CompositeWithFloatCheck->SetEnabled(0);
  this->CompositeWithRGBACheck->SetState(0);
  this->CompositeWithRGBACheck->SetEnabled(0);
}

//----------------------------------------------------------------------------
void vtkPVMultiDisplayRenderModuleUI::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

