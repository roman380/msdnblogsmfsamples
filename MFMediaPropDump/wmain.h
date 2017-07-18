// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#pragma once

HRESULT ParseParams(
    __in int iArgCount,
    __in_ecount_opt(iArgCount) WCHAR * awszArgs[],
    __in LPWSTR pwszFilePath);

void PrintHelp();

