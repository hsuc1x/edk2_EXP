/** @file
System prints Trace Hub message in PEI based on fixed PCDs and HOB.
System applies Trace Hub HOB once it detect gTraceHubDebugInfoHobGuid HOB.
Support at most MAX_TRACE_HUB_DEBUG_INSTANCE of Trace Hub debug instances
in the system.
Trace Hub PCDs will be applied if no HOB exist.

Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/PcdLib.h>
#include <Library/HobLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/TraceHubDebugSysTLib.h>
#include <Library/MipiSysTLib.h>
#include <Library/MipiSysTLib/mipi_syst.h>
#include <Guid/TraceHubDebugInfoHob.h>
#include "InternalTraceHubApiCommon.h"
#include "InternalTraceHubApi.h"

/**
  Write debug string to specified Trace Hub MMIO address.

  @param[in]  SeverityType     Severity type of input message.
  @param[in]  Buffer           A pointer to the data buffer.
  @param[in]  NumberOfBytes    The size of data buffer.

  @retval RETURN_SUCCESS      Data was written to Trace Hub.
  @retval Other               Failed to output Trace Hub message.
**/
RETURN_STATUS
EFIAPI
TraceHubSysTDebugWrite (
  IN TRACE_HUB_SEVERITY_TYPE  SeverityType,
  IN UINT8                    *Buffer,
  IN UINTN                    NumberOfBytes
  )
{
  MIPI_SYST_HANDLE  MipiSystHandle;
  MIPI_SYST_HEADER  MipiSystHeader;
  RETURN_STATUS     Status;
  UINT8             *DgbContext;
  UINTN             Index;
  UINT32            DbgInstCount;
  UINT8             *ThDebugInfo;

  DbgInstCount = 0;

  if (NumberOfBytes == 0) {
    //
    // No data need to be written to Trace Hub
    //
    return RETURN_ABORTED;
  }

  if (Buffer == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  DbgInstCount = CountThDebugInstance ();

  ZeroMem (&MipiSystHandle, sizeof (MIPI_SYST_HANDLE));
  MipiSystHandle.systh_header = &MipiSystHeader;

  Status = InitMipiSystHandle (&MipiSystHandle);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  DgbContext = (UINT8 *)GetFirstGuidHob (&gTraceHubDebugInfoHobGuid);
  if (DgbContext != NULL) {
    ThDebugInfo = GET_GUID_HOB_DATA (DgbContext);
  } else {
    ThDebugInfo = NULL;
  }

  for (Index = 0; Index < DbgInstCount; Index++) {
    Status = CheckWhetherToOutputMsg (
               &MipiSystHandle,
               ThDebugInfo,
               (MIPI_SYST_SEVERITY)SeverityType,
               TraceHubDebugType
               );
    if (!RETURN_ERROR (Status)) {
      Status = MipiSystWriteDebug (
                 &MipiSystHandle,
                 (MIPI_SYST_SEVERITY)SeverityType,
                 (UINT16)NumberOfBytes,
                 (CHAR8 *)Buffer
                 );
      if (RETURN_ERROR (Status)) {
        break;
      }
    }

    if (DgbContext != NULL) {
      DgbContext = (UINT8 *)GetNextGuidHob (&gTraceHubDebugInfoHobGuid, GET_NEXT_HOB (DgbContext));
      if (DgbContext == NULL) {
        break;
      }

      ThDebugInfo = GET_GUID_HOB_DATA (DgbContext);
    }
  }

  return Status;
}

/**
  Write catalog status code message to specified Trace Hub MMIO address.

  @param[in]  SeverityType     Severity type of input message.
  @param[in]  Id               Catalog ID.
  @param[in]  Guid             Driver Guid.

  @retval RETURN_SUCCESS      Data was written to Trace Hub.
  @retval Other               Failed to output Trace Hub message.
**/
RETURN_STATUS
EFIAPI
TraceHubSysTWriteCataLog64StatusCode (
  IN TRACE_HUB_SEVERITY_TYPE  SeverityType,
  IN UINT64                   Id,
  IN GUID                     *Guid
  )
{
  MIPI_SYST_HANDLE  MipiSystHandle;
  MIPI_SYST_HEADER  MipiSystHeader;
  UINT32            DbgInstCount;
  UINT8             *DgbContext;
  RETURN_STATUS     Status;
  UINTN             Index;
  UINT8             *ThDebugInfo;
  GUID              ConvertedGuid;

  DbgInstCount = 0;

  DbgInstCount = CountThDebugInstance ();

  MipiSystHandle.systh_header = &MipiSystHeader;

  ZeroMem (&MipiSystHandle, sizeof (MIPI_SYST_HANDLE));
  Status = InitMipiSystHandle (&MipiSystHandle);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  if (Guid != NULL) {
    ConvertedGuid = SwapBytesGuid (Guid);
    CopyMem (&MipiSystHandle.systh_guid, &ConvertedGuid, sizeof (GUID));
    MipiSystHandle.systh_tag.et_guid = 1;
  } else {
    MipiSystHandle.systh_tag.et_modunit = 2;
    MipiSystHandle.systh_tag.et_guid    = 0;
  }

  DgbContext = (UINT8 *)GetFirstGuidHob (&gTraceHubDebugInfoHobGuid);
  if (DgbContext != NULL) {
    ThDebugInfo = GET_GUID_HOB_DATA (DgbContext);
  } else {
    ThDebugInfo = NULL;
  }

  for (Index = 0; Index < DbgInstCount; Index++) {
    Status = CheckWhetherToOutputMsg (
               &MipiSystHandle,
               ThDebugInfo,
               (MIPI_SYST_SEVERITY)SeverityType,
               TraceHubCatalogType
               );
    if (!RETURN_ERROR (Status)) {
      Status = MipiSystWriteCatalog (
                 &MipiSystHandle,
                 (MIPI_SYST_SEVERITY)SeverityType,
                 Id
                 );
      if (RETURN_ERROR (Status)) {
        break;
      }
    }

    if (DgbContext != NULL) {
      DgbContext = (UINT8 *)GetNextGuidHob (&gTraceHubDebugInfoHobGuid, GET_NEXT_HOB (DgbContext));
      if (DgbContext == NULL) {
        break;
      }

      ThDebugInfo = GET_GUID_HOB_DATA (DgbContext);
    }
  }

  return Status;
}

/**
  Write catalog message to specified Trace Hub MMIO address.

  @param[in]  SeverityType   Severity type of input message.
  @param[in]  Id             Catalog ID.
  @param[in]  NumberOfParams Number of entries in argument list.
  @param[in]  ...            Catalog message parameters.

  @retval RETURN_SUCCESS      Data was written to Trace Hub.
  @retval Other               Failed to output Trace Hub message.
**/
RETURN_STATUS
EFIAPI
TraceHubSysTWriteCataLog64 (
  IN TRACE_HUB_SEVERITY_TYPE  SeverityType,
  IN UINT64                   Id,
  IN UINTN                    NumberOfParams,
  ...
  )
{
  MIPI_SYST_HANDLE  MipiSystHandle;
  MIPI_SYST_HEADER  MipiSystHeader;
  VA_LIST           Args;
  UINTN             Index;
  UINT32            DbgInstCount;
  UINT8             *DgbContext;
  RETURN_STATUS     Status;
  UINT8             *ThDebugInfo;

  DbgInstCount = 0;

  if (NumberOfParams > sizeof (MipiSystHandle.systh_param) / sizeof (UINT32)) {
    return RETURN_INVALID_PARAMETER;
  }

  DbgInstCount = CountThDebugInstance ();

  MipiSystHandle.systh_header = &MipiSystHeader;

  ZeroMem (&MipiSystHandle, sizeof (MIPI_SYST_HANDLE));
  Status = InitMipiSystHandle (&MipiSystHandle);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  MipiSystHandle.systh_param_count = (UINT32)NumberOfParams;
  VA_START (Args, NumberOfParams);
  for (Index = 0; Index < NumberOfParams; Index++) {
    MipiSystHandle.systh_param[Index] = VA_ARG (Args, UINT32);
  }

  VA_END (Args);

  DgbContext = (UINT8 *)GetFirstGuidHob (&gTraceHubDebugInfoHobGuid);
  if (DgbContext != NULL) {
    ThDebugInfo = GET_GUID_HOB_DATA (DgbContext);
  } else {
    ThDebugInfo = NULL;
  }

  for (Index = 0; Index < DbgInstCount; Index++) {
    Status = CheckWhetherToOutputMsg (
               &MipiSystHandle,
               ThDebugInfo,
               (MIPI_SYST_SEVERITY)SeverityType,
               TraceHubCatalogType
               );
    if (!RETURN_ERROR (Status)) {
      Status = MipiSystWriteCatalog (
                 &MipiSystHandle,
                 (MIPI_SYST_SEVERITY)SeverityType,
                 Id
                 );
      if (RETURN_ERROR (Status)) {
        break;
      }
    }

    if (DgbContext != NULL) {
      DgbContext = (UINT8 *)GetNextGuidHob (&gTraceHubDebugInfoHobGuid, GET_NEXT_HOB (DgbContext));
      if (DgbContext == NULL) {
        break;
      }

      ThDebugInfo = GET_GUID_HOB_DATA (DgbContext);
    }
  }

  return Status;
}
