/*
 * Copyright 2018 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "ink/pdf/form_renderer.h"

namespace ink {
namespace pdf {

namespace {
void NoopFormInvalidate(FPDF_FORMFILLINFO* pThis, FPDF_PAGE page, double l,
                        double t, double r, double b) {}
void NoopFormSetCursor(FPDF_FORMFILLINFO* pThis, int nCursorType) {}
int NoopFormSetTimer(FPDF_FORMFILLINFO* pThis, int uElapse,
                     TimerCallback lpTimerFunc) {
  return 0;
}
void NoopFormKillTimer(FPDF_FORMFILLINFO* pThis, int nTimerID) {}
FPDF_SYSTEMTIME NoopFormGetLocalTime(FPDF_FORMFILLINFO* pThis) {
  return FPDF_SYSTEMTIME();
}
FPDF_PAGE NoopFormGetPage(FPDF_FORMFILLINFO* pThis, FPDF_DOCUMENT doc,
                          int pageIndex) {
  return nullptr;
}
FPDF_PAGE NoopFormGetCurrentPage(FPDF_FORMFILLINFO* pThis, FPDF_DOCUMENT doc) {
  return nullptr;
}
int NoopFormGetRotation(FPDF_FORMFILLINFO* pThis, FPDF_PAGE page) { return 0; }
void NoopFormExecuteNamedAction(FPDF_FORMFILLINFO* pThis,
                                FPDF_BYTESTRING namedAction) {}
void NoopFormSetTextFieldFocus(FPDF_FORMFILLINFO* pThis, FPDF_WIDESTRING value,
                               FPDF_DWORD valueLen, FPDF_BOOL isFocus) {}
}  // namespace

FormRenderer::FormRenderer(FPDF_DOCUMENT doc) {
  version = 1;
  FFI_Invalidate = &NoopFormInvalidate;
  FFI_SetCursor = &NoopFormSetCursor;
  FFI_SetTimer = &NoopFormSetTimer;
  FFI_KillTimer = &NoopFormKillTimer;
  FFI_GetLocalTime = &NoopFormGetLocalTime;
  FFI_GetPage = &NoopFormGetPage;
  FFI_GetCurrentPage = &NoopFormGetCurrentPage;
  FFI_GetRotation = &NoopFormGetRotation;
  FFI_ExecuteNamedAction = &NoopFormExecuteNamedAction;
  FFI_SetTextFieldFocus = &NoopFormSetTextFieldFocus;
  m_pJsPlatform = nullptr;
  Release = nullptr;
  FFI_OnChange = nullptr;
  FFI_OutputSelectedRect = nullptr;
  FFI_DoURIAction = nullptr;
  FFI_DoGoToAction = nullptr;
  form_handle_.reset(FPDFDOC_InitFormFillEnvironment(doc, this));
}

void FormRenderer::RenderTile(FPDF_BITMAP bitmap, FPDF_PAGE page,
                              int page_width, int page_height, int tx,
                              int ty) const {
  if (form_handle_) {
    FPDF_FFLDraw(form_handle_.get(), bitmap, page, tx, ty, page_width,
                 page_height, 0, FPDF_ANNOT);
  }
}

void FormRenderer::NotifyAfterPageLoad(FPDF_PAGE page) {
  FORM_OnAfterLoadPage(page, form_handle_.get());
}

void FormRenderer::NotifyBeforePageClose(FPDF_PAGE page) {
  FORM_OnBeforeClosePage(page, form_handle_.get());
}

}  // namespace pdf
}  // namespace ink
