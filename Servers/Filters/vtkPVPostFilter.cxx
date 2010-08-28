/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkPVPostFilter.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVPostFilter.h"

#include "vtkCommand.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPVPostFilterExecutive.h"

#include "vtkDataArray.h"
#include "vtkCellData.h"
#include "vtkPointData.h"

#include "vtkCellDataToPointData.h"
#include "vtkPointDataToCellData.h"

vtkStandardNewMacro(vtkPVPostFilter);

namespace
{
  enum PropertyConversion
    {
    PointToCell = 1 << 1,
    CellToPoint = 1 << 2,
    AlsoVectorProperty = 1 << 4
    };
  enum VectorConversion
    {
    Component = 1 << 1,
    Magnitude = 1 << 2,
    };
}

//----------------------------------------------------------------------------
vtkPVPostFilter::vtkPVPostFilter()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkPVPostFilter::~vtkPVPostFilter()
{
}

//----------------------------------------------------------------------------
vtkExecutive* vtkPVPostFilter::CreateDefaultExecutive()
{
  return vtkPVPostFilterExecutive::New();
}
//----------------------------------------------------------------------------
int vtkPVPostFilter::RequestDataObject(
  vtkInformation*,
  vtkInformationVector** inputVector ,
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
    {
    return 0;
    }
  vtkDataObject *input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  if (input)
    {
    // for each output
    for(int i=0; i < this->GetNumberOfOutputPorts(); ++i)
      {
      vtkInformation* info = outputVector->GetInformationObject(i);
      vtkDataObject *output = info->Get(vtkDataObject::DATA_OBJECT());
      if (!output || !output->IsA(input->GetClassName()))
        {
        vtkDataObject* newOutput = input->NewInstance();
        newOutput->SetPipelineInformation(info);
        newOutput->Delete();
        }
      }
    return 1;
    }
  return 0;
}


//----------------------------------------------------------------------------
int vtkPVPostFilter::RequestData(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  int converted = 0;
  if (this->Information->Has(vtkPVPostFilterExecutive::POST_ARRAYS_TO_PROCESS()) )
    {
    converted = this->DetermineNeededConversion(request,inputVector,outputVector);
    }

  if (!converted)
    {
    //we need to just copy the data, since we are not needed
    vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
    vtkInformation *outInfo = outputVector->GetInformationObject(0);

    vtkDataObject* input= inInfo->Get(vtkDataObject::DATA_OBJECT());
    vtkDataObject* output= outInfo->Get(vtkDataObject::DATA_OBJECT());

    if (output && input)
      {
      output->ShallowCopy(input);
      }
    }

  return 1;
}
//----------------------------------------------------------------------------
int vtkPVPostFilter::DetermineNeededConversion(  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  int port = 0;
  vtkInformation *inInfo = inputVector[port]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkDataObject *input = inInfo->Get(vtkDataObject::DATA_OBJECT());

  //get the array to convert info
  vtkInformationVector* postVector =
    this->Information->Get(vtkPVPostFilterExecutive::POST_ARRAYS_TO_PROCESS());
  vtkInformation *postArrayInfo = postVector->GetInformationObject(0);

  if ( this->Information->Has(vtkAlgorithm::INPUT_ARRAYS_TO_PROCESS()))
    {
    //determine if the POST_ARRAYS_TO_PROCESS property
    //is already been copied
    vtkInformation *inputArrayInfo = this->GetInputArrayInformation(0);
    if ( this->MatchingPropertyInformation(inputArrayInfo,postArrayInfo) )
      {
      return 0;
      }
    //otherwise we are going to have to overwrite it
    }

  int ret = this->DeterminePointCellConversion(postArrayInfo,input);
  if ( ret & PointToCell )
    {
    vtkPointDataToCellData *converter = vtkPointDataToCellData::New();
    //we need to connect this to our inputs outputport!
    converter->SetInputConnection(0,this->GetInputConnection(port,0));
    converter->PassPointDataOn();
    converter->Update();

    vtkInformation *outInfo = outputVector->GetInformationObject(0);
    vtkDataObject* output = vtkDataObject::SafeDownCast(
      outInfo->Get(vtkDataObject::DATA_OBJECT()));
    output->ShallowCopy(converter->GetOutputDataObject(0));
    converter->Delete();
    }
  else if ( ret & CellToPoint )
    {
    vtkCellDataToPointData *converter = vtkCellDataToPointData::New();
    //we need to connect this to our inputs outputport!
    converter->SetInputConnection(0,this->GetInputConnection(port,0));
    converter->PassCellDataOn();
    converter->Update();

    vtkInformation *outInfo = outputVector->GetInformationObject(0);
    vtkDataObject* output = vtkDataObject::SafeDownCast(
      outInfo->Get(vtkDataObject::DATA_OBJECT()));
    output->ShallowCopy(converter->GetOutputDataObject(0));
    converter->Delete();
    }

  //determine if this a vector to scalar conversion
  if ( ret == 0 || ret & AlsoVectorProperty )
    {
    //the current plan is that we are going to have the name
    //of the property signify what component we should extract
    //so PROPERTY - COMPONENT NAME
    ret = this->DetermineVectorConversion(postArrayInfo,input);
    }

  return (ret != 0);
}

//----------------------------------------------------------------------------
int vtkPVPostFilter::DeterminePointCellConversion(vtkInformation *postArrayInfo,
                                                 vtkDataObject *inputObj)
{
  //determine if this is a point || cell conversion
  int retCode = 0;
  const char* name;
  int fieldAssociation = postArrayInfo->Get(vtkDataObject::FIELD_ASSOCIATION());
  vtkDataSet *dsInput = vtkDataSet::SafeDownCast(inputObj);
  if (dsInput && fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS)
    {
    name = postArrayInfo->Get(vtkDataObject::FIELD_NAME());
    int exists = dsInput->GetCellData()->HasArray(name);
    if ( exists )
      {
      retCode |= CellToPoint;
      if ( dsInput->GetCellData()->GetArray(name)->GetNumberOfTuples() > 1 )
        {
        retCode |= AlsoVectorProperty;
        }
      }

    }
  else if (dsInput && fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS)
    {
    name = postArrayInfo->Get(vtkDataObject::FIELD_NAME());
    int exists = dsInput->GetPointData()->HasArray(name);
    if ( exists )
      {
      retCode |= PointToCell;
      if ( dsInput->GetPointData()->GetArray(name)->GetNumberOfTuples() > 1 )
        {
        retCode |= AlsoVectorProperty;
        }
      }
    }
  return retCode;
}

//----------------------------------------------------------------------------
int vtkPVPostFilter::DetermineVectorConversion(vtkInformation *postArrayInfo, vtkDataObject *input)
{
  return 0;
}

//----------------------------------------------------------------------------
bool vtkPVPostFilter::MatchingPropertyInformation(
                                            vtkInformation* inputArrayInfo,
                                            vtkInformation* postArrayInfo)
{
  return (inputArrayInfo->Has(vtkDataObject::FIELD_NAME()) &&
      postArrayInfo->Has(vtkDataObject::FIELD_NAME()) &&
      inputArrayInfo->Get(INPUT_PORT()) ==
        postArrayInfo->Get(INPUT_PORT()) &&
      inputArrayInfo->Get(INPUT_CONNECTION()) ==
        postArrayInfo->Get(INPUT_CONNECTION()) &&
      inputArrayInfo->Get(vtkDataObject::FIELD_ASSOCIATION()) ==
        postArrayInfo->Get(vtkDataObject::FIELD_ASSOCIATION()) &&
      strcmp(inputArrayInfo->Get(vtkDataObject::FIELD_NAME()),
        postArrayInfo->Get(vtkDataObject::FIELD_NAME()))==0);
}

//----------------------------------------------------------------------------
void vtkPVPostFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
