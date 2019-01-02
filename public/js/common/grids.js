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

/**
 * @fileoverview Some common grid types and their data URIs
 */
goog.provide('ink.Grids');


ink.Grids = {};

/** @enum {string} */
ink.Grids.GridType = {
  NONE: 'none',
  DOTS: 'dots',
  RULES: 'rules',
  SQUARE: 'square',
};

/** @type {!Object<!ink.Grids.GridType, string>} */
ink.Grids.GRID_TYPE_TO_TEXTURE_URI = {
  [ink.Grids.GridType.DOTS]: 'sketchology://grid_dots_0',
  [ink.Grids.GridType.RULES]: 'sketchology://grid_rules_0',
  [ink.Grids.GridType.SQUARE]: 'sketchology://grid_square_0',
};

/** @type {!Object<!ink.Grids.GridType, string>} */
ink.Grids.GRID_TYPE_TO_DATA_URI = {
  [ink.Grids.GridType.NONE]: '',
  [ink.Grids.GridType.DOTS]:
      'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAgAAAAIACAM' +
      'AAADDpiTIAAAATlBMVEUAAAADAwMSEhIUFBQdHR05OTk/Pz9AQEBUVFR5' +
      'eXl6enqGhoaVlZWdnZ2lpaWurq67u7vHx8fPz8/U1NTm5ubr6+v39/f9/' +
      'f3+/v7////wLfL5AAAAGXRSTlMAAxIUHTk/QFR5eoaVnaWuu8fP1Obr9/' +
      '3+KjMbwAAAAAFiS0dEAIgFHUgAAALzSURBVBgZ7cFJUiNJFAXAr3kuCY3' +
      '57n/RXmCGgcjIfQfuXplSdCBTKhOeRQeemVCZ8Cg68MiEyoRb0YFbJlQm' +
      'nIsOnDOhMuFQdOCQCZUJ26ID20yotA3zogPzIW2VtmvRhWvaKm37ogv7t' +
      'FWahmXRheWQpkrTpejEJU2Vpk3RiU2aKi0fRTc+0lJpWRXdWKWl0nAqOn' +
      'JKQ2XcfVF0ZHHPuMqo17royvqVUZVRu6Izu4yqjDkW3TlmTGXEv1nRndm' +
      '/jKj8dpwVHZod81vl3WtXdGr3yrvKm/u66Nb6njeVn06LomOLU36qfPex' +
      'Kjq3+sh3lS/DZVP8AZvLkC+VT8N1vyz+iOX+OuRT5fm4nQ/befGnzLeH8' +
      '+3xTAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA' +
      'AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA' +
      'AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA' +
      'AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA' +
      'AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA' +
      'AAAAAAAAAAAAAAAAAAAMD/V56P2/mwnRd/ynx7ON8ez1Q+Ddf9svgjlvv' +
      'rkE+VL8NlU/wBm8uQL5XvPlZF51Yf+a7y02lRdGxxyk+VN/d10a31PW8q' +
      '7167olO7V95VfjvOig7NjvmtMuLfrOjO7F9GVMYci+4cM6Yyald0ZpdRl' +
      'VGvddGV9SujKuPui6Iji3vGVRpORUdOaai0rIpurNJSafkouvGRlkrTpu' +
      'jEJk2VpkvRiUuaKk3DsujCckhTpW1fdGGftkrbtejCNW2VtmFedGA+pK0' +
      'yYVt0YJsJlQmHogOHTKhMOBcdOGdCZcKt6MAtEyoTHkUHHplQmfAsOvDM' +
      'hMqUogOZ8h+xbAJonr/uGwAAAABJRU5ErkJggg==',
  [ink.Grids.GridType.RULES]:
      'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAEAAAABAAQMA' +
      'AACQp+OdAAAABlBMVEX///////9VfPVsAAAAAnRSTlP/AOW3MEoAAAAUSU' +
      'RBVHgBY4CD/1BAmDHKGGXAAAD0xe4gqLTwJgAAAABJRU5ErkJggg==',
  [ink.Grids.GridType.SQUARE]:
      'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAEAAAABAAQM' +
      'AAACQp+OdAAAABlBMVEX///////9VfPVsAAAAAnRSTlP/AOW3MEoAAAAW' +
      'SURBVHgBY4CD+v9g8I8wY5QxyoABAGFRzuKVnN1rAAAAAElFTkSuQmCC',
};


/**
 * Looks up grid type from the ink texture URI. Since grid users on different
 * platforms user different URIs (b/112080732#comment2), we do a fuzzy search
 * for the grid type intended by the uri.
 *
 * @param {string} uri texture URI to look up grid type for
 * @return {!ink.Grids.GridType} grid type matching URI, NONE if no match found
 */
ink.Grids.getTypeFromTextureURI = function(uri) {
  if (uri.includes('square')) {
    return ink.Grids.GridType.SQUARE;
  } else if (uri.includes('dot')) {
    return ink.Grids.GridType.DOTS;
  } else if (uri.includes('rule')) {
    return ink.Grids.GridType.RULES;
  } else {
    return ink.Grids.GridType.NONE;
  }
};

