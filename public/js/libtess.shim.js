// Copyright 2018 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

goog.require('libtess');

goog.exportSymbol('libtess.GluTesselator', libtess.GluTesselator);

goog.exportSymbol('libtess.gluEnum', libtess.gluEnum);
goog.exportProperty(libtess.gluEnum, 'GLU_TESS_BEGIN_DATA', libtess.gluEnum.GLU_TESS_BEGIN_DATA);
goog.exportProperty(libtess.gluEnum, 'GLU_TESS_EDGE_FLAG_DATA', libtess.gluEnum.GLU_TESS_EDGE_FLAG_DATA);
goog.exportProperty(libtess.gluEnum, 'GLU_TESS_VERTEX_DATA', libtess.gluEnum.GLU_TESS_VERTEX_DATA);
goog.exportProperty(libtess.gluEnum, 'GLU_TESS_COMBINE_DATA', libtess.gluEnum.GLU_TESS_COMBINE_DATA);
goog.exportProperty(libtess.gluEnum, 'GLU_TESS_END_DATA', libtess.gluEnum.GLU_TESS_END_DATA);
goog.exportProperty(libtess.gluEnum, 'GLU_TESS_ERROR_DATA', libtess.gluEnum.GLU_TESS_ERROR_DATA);


goog.exportProperty(libtess.GluTesselator.prototype, 'gluDeleteTess', libtess.GluTesselator.prototype.gluDeleteTess);
goog.exportProperty(libtess.GluTesselator.prototype, 'gluTessProperty', libtess.GluTesselator.prototype.gluTessProperty);
goog.exportProperty(libtess.GluTesselator.prototype, 'gluGetTessProperty', libtess.GluTesselator.prototype.gluGetTessProperty);
goog.exportProperty(libtess.GluTesselator.prototype, 'gluTessNormal', libtess.GluTesselator.prototype.gluTessNormal);
goog.exportProperty(libtess.GluTesselator.prototype, 'gluTessCallback', libtess.GluTesselator.prototype.gluTessCallback);
goog.exportProperty(libtess.GluTesselator.prototype, 'gluTessVertex', libtess.GluTesselator.prototype.gluTessVertex);
goog.exportProperty(libtess.GluTesselator.prototype, 'gluTessBeginPolygon', libtess.GluTesselator.prototype.gluTessBeginPolygon);
goog.exportProperty(libtess.GluTesselator.prototype, 'gluTessBeginContour', libtess.GluTesselator.prototype.gluTessBeginContour);
goog.exportProperty(libtess.GluTesselator.prototype, 'gluTessEndContour', libtess.GluTesselator.prototype.gluTessEndContour);
goog.exportProperty(libtess.GluTesselator.prototype, 'gluTessEndPolygon', libtess.GluTesselator.prototype.gluTessEndPolygon);

