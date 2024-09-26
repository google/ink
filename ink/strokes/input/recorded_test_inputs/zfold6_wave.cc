
// Copyright 2024 Google LLC
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

#include <vector>

#include "absl/log/check.h"
#include "ink/geometry/angle.h"
#include "ink/strokes/input/stroke_input.h"
#include "ink/strokes/input/stroke_input_batch.h"
#include "ink/types/duration.h"
#include "ink/types/physical_distance.h"

namespace ink {

StrokeInputBatch ZFold6Wave() {
  std::vector<StrokeInput> inputs;
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1097.7705078125, 1121.0103759765625},
      .elapsed_time = Duration32::Seconds(0.0),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.025146484375,
      .tilt = Angle::Radians(0.4443359375),
      .orientation = Angle::Radians(0.203369140625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1096.71240234375, 1118.9404296875},
      .elapsed_time = Duration32::Seconds(0.006000000052154064),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.02587890625,
      .tilt = Angle::Radians(0.46142578125),
      .orientation = Angle::Radians(0.196044921875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1096.259033203125, 1118.34912109375},
      .elapsed_time = Duration32::Seconds(0.00800000037997961),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.0263671875,
      .tilt = Angle::Radians(0.46142578125),
      .orientation = Angle::Radians(0.196044921875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1095.8055419921875, 1116.476318359375},
      .elapsed_time = Duration32::Seconds(0.009999999776482582),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.02685546875,
      .tilt = Angle::Radians(0.46142578125),
      .orientation = Angle::Radians(0.196044921875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1095.2010498046875, 1114.7021484375},
      .elapsed_time = Duration32::Seconds(0.012000000104308128),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.02734375,
      .tilt = Angle::Radians(0.46142578125),
      .orientation = Angle::Radians(0.196044921875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1094.74755859375, 1112.730712890625},
      .elapsed_time = Duration32::Seconds(0.0139999995008111),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.028076171875,
      .tilt = Angle::Radians(0.46142578125),
      .orientation = Angle::Radians(0.196044921875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1094.4453125, 1110.3651123046875},
      .elapsed_time = Duration32::Seconds(0.016999999061226845),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.028564453125,
      .tilt = Angle::Radians(0.46142578125),
      .orientation = Angle::Radians(0.196044921875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1094.4453125, 1107.703857421875},
      .elapsed_time = Duration32::Seconds(0.017999999225139618),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.029052734375,
      .tilt = Angle::Radians(0.46142578125),
      .orientation = Angle::Radians(0.196044921875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1094.4453125, 1104.451171875},
      .elapsed_time = Duration32::Seconds(0.019999999552965164),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.029541015625,
      .tilt = Angle::Radians(0.46142578125),
      .orientation = Angle::Radians(0.196044921875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1094.4453125, 1100.80419921875},
      .elapsed_time = Duration32::Seconds(0.023000000044703484),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.02978515625,
      .tilt = Angle::Radians(0.46142578125),
      .orientation = Angle::Radians(0.196044921875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1094.596435546875, 1096.5657958984375},
      .elapsed_time = Duration32::Seconds(0.02500000037252903),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.0302734375,
      .tilt = Angle::Radians(0.478515625),
      .orientation = Angle::Radians(0.189453125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1095.0499267578125, 1091.8345947265625},
      .elapsed_time = Duration32::Seconds(0.027000000700354576),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.030517578125,
      .tilt = Angle::Radians(0.478515625),
      .orientation = Angle::Radians(0.189453125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1096.259033203125, 1086.3148193359375},
      .elapsed_time = Duration32::Seconds(0.028999999165534973),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.031005859375,
      .tilt = Angle::Radians(0.478515625),
      .orientation = Angle::Radians(0.189453125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1096.71240234375, 1080.30224609375},
      .elapsed_time = Duration32::Seconds(0.03099999949336052),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.03125,
      .tilt = Angle::Radians(0.478515625),
      .orientation = Angle::Radians(0.189453125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1098.07275390625, 1073.501220703125},
      .elapsed_time = Duration32::Seconds(0.032999999821186066),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.03125,
      .tilt = Angle::Radians(0.478515625),
      .orientation = Angle::Radians(0.189453125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1099.4329833984375, 1066.3057861328125},
      .elapsed_time = Duration32::Seconds(0.03500000014901161),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.031494140625,
      .tilt = Angle::Radians(0.478515625),
      .orientation = Angle::Radians(0.189453125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1100.6422119140625, 1058.51904296875},
      .elapsed_time = Duration32::Seconds(0.03700000047683716),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.03173828125,
      .tilt = Angle::Radians(0.478515625),
      .orientation = Angle::Radians(0.189453125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1102.153564453125, 1050.140869140625},
      .elapsed_time = Duration32::Seconds(0.039000000804662704),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.031982421875,
      .tilt = Angle::Radians(0.478515625),
      .orientation = Angle::Radians(0.189453125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1103.6650390625, 1041.368408203125},
      .elapsed_time = Duration32::Seconds(0.04100000113248825),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.0322265625,
      .tilt = Angle::Radians(0.478515625),
      .orientation = Angle::Radians(0.189453125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1105.478759765625, 1032.1031494140625},
      .elapsed_time = Duration32::Seconds(0.0430000014603138),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.0322265625,
      .tilt = Angle::Radians(0.485595703125),
      .orientation = Angle::Radians(0.26220703125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1107.29248046875, 1022.6407470703125},
      .elapsed_time = Duration32::Seconds(0.04600000008940697),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.032470703125,
      .tilt = Angle::Radians(0.50244140625),
      .orientation = Angle::Radians(0.25390625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1109.408447265625, 1012.7840576171875},
      .elapsed_time = Duration32::Seconds(0.04800000041723251),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.03271484375,
      .tilt = Angle::Radians(0.50244140625),
      .orientation = Angle::Radians(0.25390625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1111.675537109375, 1002.5331420898438},
      .elapsed_time = Duration32::Seconds(0.05000000074505806),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.03271484375,
      .tilt = Angle::Radians(0.50244140625),
      .orientation = Angle::Radians(0.25390625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1113.79150390625, 991.887939453125},
      .elapsed_time = Duration32::Seconds(0.052000001072883606),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.032958984375,
      .tilt = Angle::Radians(0.50244140625),
      .orientation = Angle::Radians(0.25390625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1116.0587158203125, 981.1441040039062},
      .elapsed_time = Duration32::Seconds(0.05400000140070915),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.032958984375,
      .tilt = Angle::Radians(0.50244140625),
      .orientation = Angle::Radians(0.25390625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1118.3258056640625, 970.1046752929688},
      .elapsed_time = Duration32::Seconds(0.0559999980032444),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.033203125,
      .tilt = Angle::Radians(0.50244140625),
      .orientation = Angle::Radians(0.25390625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1120.744140625, 958.8680419921875},
      .elapsed_time = Duration32::Seconds(0.057999998331069946),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.033447265625,
      .tilt = Angle::Radians(0.50244140625),
      .orientation = Angle::Radians(0.25390625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1123.3135986328125, 947.5328369140625},
      .elapsed_time = Duration32::Seconds(0.05999999865889549),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.033447265625,
      .tilt = Angle::Radians(0.50244140625),
      .orientation = Angle::Radians(0.25390625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1125.7318115234375, 934.7191772460938},
      .elapsed_time = Duration32::Seconds(0.06199999898672104),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.03369140625,
      .tilt = Angle::Radians(0.50244140625),
      .orientation = Angle::Radians(0.25390625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1130.114990234375, 913.4287719726562},
      .elapsed_time = Duration32::Seconds(0.06400000303983688),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.04736328125,
      .tilt = Angle::Radians(0.50244140625),
      .orientation = Angle::Radians(0.25390625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1133.742431640625, 895.29248046875},
      .elapsed_time = Duration32::Seconds(0.06599999964237213),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.05126953125,
      .tilt = Angle::Radians(0.519287109375),
      .orientation = Angle::Radians(0.24609375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1136.7652587890625, 880.310302734375},
      .elapsed_time = Duration32::Seconds(0.06799999624490738),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.056396484375,
      .tilt = Angle::Radians(0.519287109375),
      .orientation = Angle::Radians(0.24609375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1138.8812255859375, 870.2565307617188},
      .elapsed_time = Duration32::Seconds(0.07000000029802322),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.0634765625,
      .tilt = Angle::Radians(0.519287109375),
      .orientation = Angle::Radians(0.24609375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1140.8460693359375, 860.9912109375},
      .elapsed_time = Duration32::Seconds(0.07199999690055847),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.071044921875,
      .tilt = Angle::Radians(0.519287109375),
      .orientation = Angle::Radians(0.24609375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1143.11328125, 852.1202392578125},
      .elapsed_time = Duration32::Seconds(0.07500000298023224),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.079345703125,
      .tilt = Angle::Radians(0.519287109375),
      .orientation = Angle::Radians(0.24609375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1145.229248046875, 844.0377807617188},
      .elapsed_time = Duration32::Seconds(0.07699999958276749),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.08740234375,
      .tilt = Angle::Radians(0.519287109375),
      .orientation = Angle::Radians(0.24609375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1147.194091796875, 836.5466918945312},
      .elapsed_time = Duration32::Seconds(0.07899999618530273),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.094482421875,
      .tilt = Angle::Radians(0.519287109375),
      .orientation = Angle::Radians(0.24609375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1148.856689453125, 829.7455444335938},
      .elapsed_time = Duration32::Seconds(0.08100000023841858),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.101318359375,
      .tilt = Angle::Radians(0.519287109375),
      .orientation = Angle::Radians(0.24609375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1150.2169189453125, 823.535888671875},
      .elapsed_time = Duration32::Seconds(0.08299999684095383),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.107421875,
      .tilt = Angle::Radians(0.519287109375),
      .orientation = Angle::Radians(0.24609375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1151.5772705078125, 818.1146850585938},
      .elapsed_time = Duration32::Seconds(0.08500000089406967),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.112548828125,
      .tilt = Angle::Radians(0.519287109375),
      .orientation = Angle::Radians(0.24609375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1153.088623046875, 813.0877685546875},
      .elapsed_time = Duration32::Seconds(0.08699999749660492),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.11767578125,
      .tilt = Angle::Radians(0.5361328125),
      .orientation = Angle::Radians(0.239013671875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1154.448974609375, 808.947998046875},
      .elapsed_time = Duration32::Seconds(0.08900000154972076),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.120849609375,
      .tilt = Angle::Radians(0.5361328125),
      .orientation = Angle::Radians(0.239013671875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1155.8092041015625, 805.3995971679688},
      .elapsed_time = Duration32::Seconds(0.09099999815225601),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.122802734375,
      .tilt = Angle::Radians(0.5361328125),
      .orientation = Angle::Radians(0.239013671875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1157.018310546875, 802.343994140625},
      .elapsed_time = Duration32::Seconds(0.09300000220537186),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.12451171875,
      .tilt = Angle::Radians(0.5361328125),
      .orientation = Angle::Radians(0.239013671875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1157.4718017578125, 799.78125},
      .elapsed_time = Duration32::Seconds(0.0949999988079071),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.124755859375,
      .tilt = Angle::Radians(0.5361328125),
      .orientation = Angle::Radians(0.239013671875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1159.1343994140625, 797.9085083007812},
      .elapsed_time = Duration32::Seconds(0.09700000286102295),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.1240234375,
      .tilt = Angle::Radians(0.5361328125),
      .orientation = Angle::Radians(0.239013671875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1159.5877685546875, 796.2328491210938},
      .elapsed_time = Duration32::Seconds(0.0989999994635582),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.12255859375,
      .tilt = Angle::Radians(0.5361328125),
      .orientation = Angle::Radians(0.239013671875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1160.947998046875, 795.838623046875},
      .elapsed_time = Duration32::Seconds(0.10099999606609344),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.11962890625,
      .tilt = Angle::Radians(0.5361328125),
      .orientation = Angle::Radians(0.239013671875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1161.4014892578125, 795.5429077148438},
      .elapsed_time = Duration32::Seconds(0.10400000214576721),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.116943359375,
      .tilt = Angle::Radians(0.5400390625),
      .orientation = Angle::Radians(0.271240234375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1162.76171875, 795.4443359375},
      .elapsed_time = Duration32::Seconds(0.10599999874830246),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.11474609375,
      .tilt = Angle::Radians(0.5400390625),
      .orientation = Angle::Radians(0.271240234375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1163.2152099609375, 795.4443359375},
      .elapsed_time = Duration32::Seconds(0.1080000028014183),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.113525390625,
      .tilt = Angle::Radians(0.5400390625),
      .orientation = Angle::Radians(0.271240234375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1164.575439453125, 795.838623046875},
      .elapsed_time = Duration32::Seconds(0.10999999940395355),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.11328125,
      .tilt = Angle::Radians(0.5400390625),
      .orientation = Angle::Radians(0.271240234375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1164.8778076171875, 797.7113647460938},
      .elapsed_time = Duration32::Seconds(0.1119999960064888),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.11376953125,
      .tilt = Angle::Radians(0.5400390625),
      .orientation = Angle::Radians(0.271240234375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1166.0869140625, 799.6826782226562},
      .elapsed_time = Duration32::Seconds(0.11400000005960464),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.116455078125,
      .tilt = Angle::Radians(0.57373046875),
      .orientation = Angle::Radians(0.256591796875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1166.38916015625, 802.6397094726562},
      .elapsed_time = Duration32::Seconds(0.11599999666213989),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.1220703125,
      .tilt = Angle::Radians(0.57373046875),
      .orientation = Angle::Radians(0.256591796875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1166.8426513671875, 806.2866821289062},
      .elapsed_time = Duration32::Seconds(0.11800000071525574),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.131103515625,
      .tilt = Angle::Radians(0.57373046875),
      .orientation = Angle::Radians(0.256591796875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1168.0517578125, 810.9193115234375},
      .elapsed_time = Duration32::Seconds(0.11999999731779099),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.1435546875,
      .tilt = Angle::Radians(0.57373046875),
      .orientation = Angle::Radians(0.256591796875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1168.0517578125, 816.7347412109375},
      .elapsed_time = Duration32::Seconds(0.12200000137090683),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.157958984375,
      .tilt = Angle::Radians(0.57373046875),
      .orientation = Angle::Radians(0.256591796875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1168.505126953125, 823.83154296875},
      .elapsed_time = Duration32::Seconds(0.12399999797344208),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.173583984375,
      .tilt = Angle::Radians(0.57373046875),
      .orientation = Angle::Radians(0.256591796875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1168.9586181640625, 831.9140625},
      .elapsed_time = Duration32::Seconds(0.12600000202655792),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.18798828125,
      .tilt = Angle::Radians(0.57373046875),
      .orientation = Angle::Radians(0.256591796875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1169.4119873046875, 840.982177734375},
      .elapsed_time = Duration32::Seconds(0.12800000607967377),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.200927734375,
      .tilt = Angle::Radians(0.57373046875),
      .orientation = Angle::Radians(0.256591796875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1170.6212158203125, 851.1345825195312},
      .elapsed_time = Duration32::Seconds(0.12999999523162842),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.21240234375,
      .tilt = Angle::Radians(0.57373046875),
      .orientation = Angle::Radians(0.256591796875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1170.6212158203125, 861.9769287109375},
      .elapsed_time = Duration32::Seconds(0.13300000131130219),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.2197265625,
      .tilt = Angle::Radians(0.57373046875),
      .orientation = Angle::Radians(0.256591796875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1171.67919921875, 873.804931640625},
      .elapsed_time = Duration32::Seconds(0.13500000536441803),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.2265625,
      .tilt = Angle::Radians(0.590576171875),
      .orientation = Angle::Radians(0.25),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1171.9814453125, 886.52001953125},
      .elapsed_time = Duration32::Seconds(0.13699999451637268),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.231689453125,
      .tilt = Angle::Radians(0.590576171875),
      .orientation = Angle::Radians(0.25),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1173.492919921875, 899.62939453125},
      .elapsed_time = Duration32::Seconds(0.13899999856948853),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.236083984375,
      .tilt = Angle::Radians(0.590576171875),
      .orientation = Angle::Radians(0.25),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1174.7020263671875, 913.6259155273438},
      .elapsed_time = Duration32::Seconds(0.14100000262260437),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.240966796875,
      .tilt = Angle::Radians(0.590576171875),
      .orientation = Angle::Radians(0.25),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1176.062255859375, 927.720947265625},
      .elapsed_time = Duration32::Seconds(0.14300000667572021),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.2431640625,
      .tilt = Angle::Radians(0.590576171875),
      .orientation = Angle::Radians(0.25),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1177.724853515625, 942.308837890625},
      .elapsed_time = Duration32::Seconds(0.14499999582767487),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.24560546875,
      .tilt = Angle::Radians(0.590576171875),
      .orientation = Angle::Radians(0.25),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1179.689697265625, 957.1923828125},
      .elapsed_time = Duration32::Seconds(0.1469999998807907),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.246826171875,
      .tilt = Angle::Radians(0.590576171875),
      .orientation = Angle::Radians(0.25),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1181.654541015625, 971.8788452148438},
      .elapsed_time = Duration32::Seconds(0.14900000393390656),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.249755859375,
      .tilt = Angle::Radians(0.590576171875),
      .orientation = Angle::Radians(0.25),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1183.7706298828125, 986.5653076171875},
      .elapsed_time = Duration32::Seconds(0.1509999930858612),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.25390625,
      .tilt = Angle::Radians(0.590576171875),
      .orientation = Angle::Radians(0.25),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1185.8865966796875, 1001.4489135742188},
      .elapsed_time = Duration32::Seconds(0.15299999713897705),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.258544921875,
      .tilt = Angle::Radians(0.590576171875),
      .orientation = Angle::Radians(0.25),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1188.3048095703125, 1016.23388671875},
      .elapsed_time = Duration32::Seconds(0.1550000011920929),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.263427734375,
      .tilt = Angle::Radians(0.590576171875),
      .orientation = Angle::Radians(0.25),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1190.874267578125, 1030.92041015625},
      .elapsed_time = Duration32::Seconds(0.15700000524520874),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.266845703125,
      .tilt = Angle::Radians(0.590576171875),
      .orientation = Angle::Radians(0.25),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1193.5948486328125, 1045.40966796875},
      .elapsed_time = Duration32::Seconds(0.1599999964237213),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.2705078125,
      .tilt = Angle::Radians(0.590576171875),
      .orientation = Angle::Radians(0.25),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1196.61767578125, 1059.5047607421875},
      .elapsed_time = Duration32::Seconds(0.16200000047683716),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.277587890625,
      .tilt = Angle::Radians(0.607666015625),
      .orientation = Angle::Radians(0.243896484375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1199.791748046875, 1072.9097900390625},
      .elapsed_time = Duration32::Seconds(0.164000004529953),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.28662109375,
      .tilt = Angle::Radians(0.607666015625),
      .orientation = Angle::Radians(0.243896484375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1203.1168212890625, 1085.920654296875},
      .elapsed_time = Duration32::Seconds(0.16599999368190765),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.2978515625,
      .tilt = Angle::Radians(0.604248046875),
      .orientation = Angle::Radians(0.21435546875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1206.4420166015625, 1098.3399658203125},
      .elapsed_time = Duration32::Seconds(0.1679999977350235),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.30615234375,
      .tilt = Angle::Radians(0.604248046875),
      .orientation = Angle::Radians(0.21435546875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1209.76708984375, 1110.2666015625},
      .elapsed_time = Duration32::Seconds(0.17000000178813934),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.31201171875,
      .tilt = Angle::Radians(0.604248046875),
      .orientation = Angle::Radians(0.21435546875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1213.39453125, 1121.4046630859375},
      .elapsed_time = Duration32::Seconds(0.1720000058412552),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.317138671875,
      .tilt = Angle::Radians(0.604248046875),
      .orientation = Angle::Radians(0.21435546875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1217.173095703125, 1131.6556396484375},
      .elapsed_time = Duration32::Seconds(0.17399999499320984),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.3203125,
      .tilt = Angle::Radians(0.604248046875),
      .orientation = Angle::Radians(0.21435546875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1220.95166015625, 1141.0194091796875},
      .elapsed_time = Duration32::Seconds(0.17599999904632568),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.326171875,
      .tilt = Angle::Radians(0.604248046875),
      .orientation = Angle::Radians(0.21435546875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1224.5791015625, 1149.4962158203125},
      .elapsed_time = Duration32::Seconds(0.17800000309944153),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.32763671875,
      .tilt = Angle::Radians(0.604248046875),
      .orientation = Angle::Radians(0.21435546875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1227.9041748046875, 1157.0858154296875},
      .elapsed_time = Duration32::Seconds(0.17999999225139618),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.327392578125,
      .tilt = Angle::Radians(0.604248046875),
      .orientation = Angle::Radians(0.21435546875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1230.77587890625, 1164.1826171875},
      .elapsed_time = Duration32::Seconds(0.18199999630451202),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.326904296875,
      .tilt = Angle::Radians(0.604248046875),
      .orientation = Angle::Radians(0.21435546875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1233.4964599609375, 1170.1951904296875},
      .elapsed_time = Duration32::Seconds(0.18400000035762787),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.3251953125,
      .tilt = Angle::Radians(0.604248046875),
      .orientation = Angle::Radians(0.21435546875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1236.3681640625, 1175.419189453125},
      .elapsed_time = Duration32::Seconds(0.1860000044107437),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.324462890625,
      .tilt = Angle::Radians(0.604248046875),
      .orientation = Angle::Radians(0.21435546875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1238.7864990234375, 1179.6575927734375},
      .elapsed_time = Duration32::Seconds(0.1889999955892563),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.323974609375,
      .tilt = Angle::Radians(0.604248046875),
      .orientation = Angle::Radians(0.21435546875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1241.35595703125, 1183.0089111328125},
      .elapsed_time = Duration32::Seconds(0.19099999964237213),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.32373046875,
      .tilt = Angle::Radians(0.604248046875),
      .orientation = Angle::Radians(0.21435546875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1243.471923828125, 1185.37451171875},
      .elapsed_time = Duration32::Seconds(0.19300000369548798),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.3232421875,
      .tilt = Angle::Radians(0.598876953125),
      .orientation = Angle::Radians(0.154541015625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1245.587890625, 1187.3458251953125},
      .elapsed_time = Duration32::Seconds(0.19499999284744263),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.32373046875,
      .tilt = Angle::Radians(0.598876953125),
      .orientation = Angle::Radians(0.154541015625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1247.552734375, 1188.627197265625},
      .elapsed_time = Duration32::Seconds(0.19699999690055847),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.32373046875,
      .tilt = Angle::Radians(0.598876953125),
      .orientation = Angle::Radians(0.154541015625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1249.064208984375, 1188.9228515625},
      .elapsed_time = Duration32::Seconds(0.19900000095367432),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.322265625,
      .tilt = Angle::Radians(0.581787109375),
      .orientation = Angle::Radians(0.158447265625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1250.5755615234375, 1189.415771484375},
      .elapsed_time = Duration32::Seconds(0.20100000500679016),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.321533203125,
      .tilt = Angle::Radians(0.581787109375),
      .orientation = Angle::Radians(0.158447265625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1252.0870361328125, 1189.71142578125},
      .elapsed_time = Duration32::Seconds(0.2029999941587448),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.32080078125,
      .tilt = Angle::Radians(0.581787109375),
      .orientation = Angle::Radians(0.158447265625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1253.447265625, 1189.71142578125},
      .elapsed_time = Duration32::Seconds(0.20499999821186066),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.3212890625,
      .tilt = Angle::Radians(0.581787109375),
      .orientation = Angle::Radians(0.158447265625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1254.656494140625, 1189.5142822265625},
      .elapsed_time = Duration32::Seconds(0.2070000022649765),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.322265625,
      .tilt = Angle::Radians(0.581787109375),
      .orientation = Angle::Radians(0.158447265625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1255.8656005859375, 1188.23291015625},
      .elapsed_time = Duration32::Seconds(0.20900000631809235),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.323486328125,
      .tilt = Angle::Radians(0.581787109375),
      .orientation = Angle::Radians(0.158447265625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1257.07470703125, 1187.0501708984375},
      .elapsed_time = Duration32::Seconds(0.210999995470047),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.32373046875,
      .tilt = Angle::Radians(0.564453125),
      .orientation = Angle::Radians(0.162841796875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1258.283935546875, 1185.37451171875},
      .elapsed_time = Duration32::Seconds(0.21299999952316284),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.322998046875,
      .tilt = Angle::Radians(0.562255859375),
      .orientation = Angle::Radians(0.130859375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1259.4930419921875, 1183.3045654296875},
      .elapsed_time = Duration32::Seconds(0.2160000056028366),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.32177734375,
      .tilt = Angle::Radians(0.562255859375),
      .orientation = Angle::Radians(0.130859375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1259.9464111328125, 1180.2490234375},
      .elapsed_time = Duration32::Seconds(0.21799999475479126),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.3212890625,
      .tilt = Angle::Radians(0.562255859375),
      .orientation = Angle::Radians(0.130859375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1261.3067626953125, 1176.306396484375},
      .elapsed_time = Duration32::Seconds(0.2199999988079071),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.321533203125,
      .tilt = Angle::Radians(0.562255859375),
      .orientation = Angle::Radians(0.130859375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1261.7601318359375, 1171.3780517578125},
      .elapsed_time = Duration32::Seconds(0.22200000286102295),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.32177734375,
      .tilt = Angle::Radians(0.562255859375),
      .orientation = Angle::Radians(0.130859375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1263.2716064453125, 1165.6611328125},
      .elapsed_time = Duration32::Seconds(0.2239999920129776),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.321044921875,
      .tilt = Angle::Radians(0.562255859375),
      .orientation = Angle::Radians(0.130859375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1263.7249755859375, 1158.761474609375},
      .elapsed_time = Duration32::Seconds(0.22599999606609344),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.31982421875,
      .tilt = Angle::Radians(0.562255859375),
      .orientation = Angle::Radians(0.130859375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1264.9342041015625, 1150.8760986328125},
      .elapsed_time = Duration32::Seconds(0.2280000001192093),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.3173828125,
      .tilt = Angle::Radians(0.562255859375),
      .orientation = Angle::Radians(0.130859375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1265.2364501953125, 1142.1036376953125},
      .elapsed_time = Duration32::Seconds(0.23000000417232513),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.314453125,
      .tilt = Angle::Radians(0.562255859375),
      .orientation = Angle::Radians(0.130859375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1266.445556640625, 1132.3455810546875},
      .elapsed_time = Duration32::Seconds(0.23199999332427979),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.3125,
      .tilt = Angle::Radians(0.562255859375),
      .orientation = Angle::Radians(0.130859375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1266.7479248046875, 1121.99609375},
      .elapsed_time = Duration32::Seconds(0.23399999737739563),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.310302734375,
      .tilt = Angle::Radians(0.527587890625),
      .orientation = Angle::Radians(0.138427734375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1267.805908203125, 1110.8580322265625},
      .elapsed_time = Duration32::Seconds(0.23600000143051147),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.30859375,
      .tilt = Angle::Radians(0.527587890625),
      .orientation = Angle::Radians(0.138427734375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1268.108154296875, 1099.2271728515625},
      .elapsed_time = Duration32::Seconds(0.23800000548362732),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.306640625,
      .tilt = Angle::Radians(0.527587890625),
      .orientation = Angle::Radians(0.138427734375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1269.4683837890625, 1086.610595703125},
      .elapsed_time = Duration32::Seconds(0.23999999463558197),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.302001953125,
      .tilt = Angle::Radians(0.527587890625),
      .orientation = Angle::Radians(0.138427734375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1269.921875, 1073.796875},
      .elapsed_time = Duration32::Seconds(0.24300000071525574),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.29736328125,
      .tilt = Angle::Radians(0.527587890625),
      .orientation = Angle::Radians(0.138427734375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1271.1309814453125, 1060.293212890625},
      .elapsed_time = Duration32::Seconds(0.24500000476837158),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.292236328125,
      .tilt = Angle::Radians(0.527587890625),
      .orientation = Angle::Radians(0.138427734375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1271.58447265625, 1046.2967529296875},
      .elapsed_time = Duration32::Seconds(0.24699999392032623),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.28857421875,
      .tilt = Angle::Radians(0.527587890625),
      .orientation = Angle::Radians(0.138427734375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1273.2469482421875, 1031.8074951171875},
      .elapsed_time = Duration32::Seconds(0.24899999797344208),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.285400390625,
      .tilt = Angle::Radians(0.527587890625),
      .orientation = Angle::Radians(0.138427734375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1273.700439453125, 1016.8253173828125},
      .elapsed_time = Duration32::Seconds(0.25099998712539673),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.277587890625,
      .tilt = Angle::Radians(0.527587890625),
      .orientation = Angle::Radians(0.138427734375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1275.51416015625, 1001.4489135742188},
      .elapsed_time = Duration32::Seconds(0.2529999911785126),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.267578125,
      .tilt = Angle::Radians(0.527587890625),
      .orientation = Angle::Radians(0.138427734375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1276.8743896484375, 985.5796508789062},
      .elapsed_time = Duration32::Seconds(0.2549999952316284),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.255859375,
      .tilt = Angle::Radians(0.510498046875),
      .orientation = Angle::Radians(0.142822265625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1278.2347412109375, 969.61181640625},
      .elapsed_time = Duration32::Seconds(0.25699999928474426),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.244873046875,
      .tilt = Angle::Radians(0.510498046875),
      .orientation = Angle::Radians(0.142822265625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1279.74609375, 953.8411254882812},
      .elapsed_time = Duration32::Seconds(0.2590000033378601),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.2373046875,
      .tilt = Angle::Radians(0.510498046875),
      .orientation = Angle::Radians(0.142822265625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1281.40869140625, 937.9718627929688},
      .elapsed_time = Duration32::Seconds(0.26100000739097595),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.2314453125,
      .tilt = Angle::Radians(0.510498046875),
      .orientation = Angle::Radians(0.142822265625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1283.222412109375, 922.2012329101562},
      .elapsed_time = Duration32::Seconds(0.2630000114440918),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.226806640625,
      .tilt = Angle::Radians(0.510498046875),
      .orientation = Angle::Radians(0.142822265625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1284.885009765625, 906.5291137695312},
      .elapsed_time = Duration32::Seconds(0.26499998569488525),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.222900390625,
      .tilt = Angle::Radians(0.510498046875),
      .orientation = Angle::Radians(0.142822265625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1286.69873046875, 891.251220703125},
      .elapsed_time = Duration32::Seconds(0.2669999897480011),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.220458984375,
      .tilt = Angle::Radians(0.510498046875),
      .orientation = Angle::Radians(0.142822265625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1288.814697265625, 876.7619018554688},
      .elapsed_time = Duration32::Seconds(0.26899999380111694),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.22021484375,
      .tilt = Angle::Radians(0.510498046875),
      .orientation = Angle::Radians(0.142822265625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1290.779541015625, 863.4553833007812},
      .elapsed_time = Duration32::Seconds(0.2709999978542328),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.219970703125,
      .tilt = Angle::Radians(0.510498046875),
      .orientation = Angle::Radians(0.142822265625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1292.744384765625, 851.52880859375},
      .elapsed_time = Duration32::Seconds(0.27399998903274536),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.22021484375,
      .tilt = Angle::Radians(0.510498046875),
      .orientation = Angle::Radians(0.142822265625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1294.8603515625, 840.982177734375},
      .elapsed_time = Duration32::Seconds(0.2759999930858612),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.220703125,
      .tilt = Angle::Radians(0.510498046875),
      .orientation = Angle::Radians(0.142822265625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1296.674072265625, 831.6183471679688},
      .elapsed_time = Duration32::Seconds(0.27799999713897705),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.219970703125,
      .tilt = Angle::Radians(0.510498046875),
      .orientation = Angle::Radians(0.142822265625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1298.638916015625, 823.7329711914062},
      .elapsed_time = Duration32::Seconds(0.2800000011920929),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.220703125,
      .tilt = Angle::Radians(0.510498046875),
      .orientation = Angle::Radians(0.142822265625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1300.45263671875, 817.0304565429688},
      .elapsed_time = Duration32::Seconds(0.28200000524520874),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.221435546875,
      .tilt = Angle::Radians(0.510498046875),
      .orientation = Angle::Radians(0.142822265625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1302.115234375, 811.9049682617188},
      .elapsed_time = Duration32::Seconds(0.2840000092983246),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.22265625,
      .tilt = Angle::Radians(0.510498046875),
      .orientation = Angle::Radians(0.142822265625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1303.77783203125, 807.7651977539062},
      .elapsed_time = Duration32::Seconds(0.28600001335144043),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.224365234375,
      .tilt = Angle::Radians(0.512939453125),
      .orientation = Angle::Radians(0.177734375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1305.591552734375, 804.512451171875},
      .elapsed_time = Duration32::Seconds(0.2879999876022339),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.224365234375,
      .tilt = Angle::Radians(0.512939453125),
      .orientation = Angle::Radians(0.177734375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1307.254150390625, 802.1468505859375},
      .elapsed_time = Duration32::Seconds(0.28999999165534973),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.22265625,
      .tilt = Angle::Radians(0.512939453125),
      .orientation = Angle::Radians(0.177734375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1308.9166259765625, 800.56982421875},
      .elapsed_time = Duration32::Seconds(0.2919999957084656),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.21923828125,
      .tilt = Angle::Radians(0.512939453125),
      .orientation = Angle::Radians(0.177734375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1310.2769775390625, 800.0769653320312},
      .elapsed_time = Duration32::Seconds(0.2939999997615814),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.212890625,
      .tilt = Angle::Radians(0.512939453125),
      .orientation = Angle::Radians(0.177734375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1311.63720703125, 799.6826782226562},
      .elapsed_time = Duration32::Seconds(0.29600000381469727),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.20556640625,
      .tilt = Angle::Radians(0.512939453125),
      .orientation = Angle::Radians(0.177734375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1312.99755859375, 799.6826782226562},
      .elapsed_time = Duration32::Seconds(0.2980000078678131),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.198486328125,
      .tilt = Angle::Radians(0.512939453125),
      .orientation = Angle::Radians(0.177734375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1314.3577880859375, 800.0769653320312},
      .elapsed_time = Duration32::Seconds(0.3009999990463257),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.193359375,
      .tilt = Angle::Radians(0.512939453125),
      .orientation = Angle::Radians(0.177734375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1315.415771484375, 802.1468505859375},
      .elapsed_time = Duration32::Seconds(0.30300000309944153),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.19091796875,
      .tilt = Angle::Radians(0.512939453125),
      .orientation = Angle::Radians(0.177734375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1316.776123046875, 804.512451171875},
      .elapsed_time = Duration32::Seconds(0.3050000071525574),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.19140625,
      .tilt = Angle::Radians(0.512939453125),
      .orientation = Angle::Radians(0.177734375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1317.9852294921875, 807.9623413085938},
      .elapsed_time = Duration32::Seconds(0.3070000112056732),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.194091796875,
      .tilt = Angle::Radians(0.512939453125),
      .orientation = Angle::Radians(0.177734375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1319.1943359375, 812.4963989257812},
      .elapsed_time = Duration32::Seconds(0.3089999854564667),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.197265625,
      .tilt = Angle::Radians(0.512939453125),
      .orientation = Angle::Radians(0.177734375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1320.4034423828125, 818.410400390625},
      .elapsed_time = Duration32::Seconds(0.3109999895095825),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.2041015625,
      .tilt = Angle::Radians(0.512939453125),
      .orientation = Angle::Radians(0.177734375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1321.4615478515625, 825.6057739257812},
      .elapsed_time = Duration32::Seconds(0.31299999356269836),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.21337890625,
      .tilt = Angle::Radians(0.512939453125),
      .orientation = Angle::Radians(0.177734375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1322.670654296875, 834.1810913085938},
      .elapsed_time = Duration32::Seconds(0.3149999976158142),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.2236328125,
      .tilt = Angle::Radians(0.519287109375),
      .orientation = Angle::Radians(0.24609375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1323.8797607421875, 843.7420654296875},
      .elapsed_time = Duration32::Seconds(0.31700000166893005),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.235595703125,
      .tilt = Angle::Radians(0.519287109375),
      .orientation = Angle::Radians(0.24609375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1325.2401123046875, 854.48583984375},
      .elapsed_time = Duration32::Seconds(0.3190000057220459),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.246826171875,
      .tilt = Angle::Radians(0.519287109375),
      .orientation = Angle::Radians(0.24609375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1326.600341796875, 866.4124145507812},
      .elapsed_time = Duration32::Seconds(0.32100000977516174),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.257568359375,
      .tilt = Angle::Radians(0.519287109375),
      .orientation = Angle::Radians(0.24609375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1328.11181640625, 879.4232177734375},
      .elapsed_time = Duration32::Seconds(0.3240000009536743),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.268310546875,
      .tilt = Angle::Radians(0.50244140625),
      .orientation = Angle::Radians(0.25390625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1329.7742919921875, 893.7153930664062},
      .elapsed_time = Duration32::Seconds(0.32499998807907104),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.2783203125,
      .tilt = Angle::Radians(0.50244140625),
      .orientation = Angle::Radians(0.25390625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1331.5880126953125, 908.9932861328125},
      .elapsed_time = Duration32::Seconds(0.3269999921321869),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.286376953125,
      .tilt = Angle::Radians(0.50244140625),
      .orientation = Angle::Radians(0.25390625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1333.5528564453125, 924.9610595703125},
      .elapsed_time = Duration32::Seconds(0.33000001311302185),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.2919921875,
      .tilt = Angle::Radians(0.50244140625),
      .orientation = Angle::Radians(0.25390625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1336.122314453125, 941.1260375976562},
      .elapsed_time = Duration32::Seconds(0.3319999873638153),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.29541015625,
      .tilt = Angle::Radians(0.50244140625),
      .orientation = Angle::Radians(0.25390625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1338.8428955078125, 957.6852416992188},
      .elapsed_time = Duration32::Seconds(0.33399999141693115),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.295654296875,
      .tilt = Angle::Radians(0.50244140625),
      .orientation = Angle::Radians(0.25390625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1342.016845703125, 973.8502197265625},
      .elapsed_time = Duration32::Seconds(0.335999995470047),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.294921875,
      .tilt = Angle::Radians(0.50244140625),
      .orientation = Angle::Radians(0.25390625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1345.342041015625, 989.620849609375},
      .elapsed_time = Duration32::Seconds(0.33799999952316284),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.29345703125,
      .tilt = Angle::Radians(0.50244140625),
      .orientation = Angle::Radians(0.25390625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1348.818359375, 1005.5886840820312},
      .elapsed_time = Duration32::Seconds(0.3400000035762787),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.292236328125,
      .tilt = Angle::Radians(0.50244140625),
      .orientation = Angle::Radians(0.25390625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1352.596923828125, 1021.2608032226562},
      .elapsed_time = Duration32::Seconds(0.34200000762939453),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.291748046875,
      .tilt = Angle::Radians(0.49560546875),
      .orientation = Angle::Radians(0.183349609375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1356.526611328125, 1036.73583984375},
      .elapsed_time = Duration32::Seconds(0.3440000116825104),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.291748046875,
      .tilt = Angle::Radians(0.49560546875),
      .orientation = Angle::Radians(0.183349609375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1360.758544921875, 1052.013671875},
      .elapsed_time = Duration32::Seconds(0.34599998593330383),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.291015625,
      .tilt = Angle::Radians(0.49560546875),
      .orientation = Angle::Radians(0.183349609375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1365.2928466796875, 1066.7987060546875},
      .elapsed_time = Duration32::Seconds(0.3479999899864197),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.289794921875,
      .tilt = Angle::Radians(0.49560546875),
      .orientation = Angle::Radians(0.183349609375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1369.978271484375, 1080.795166015625},
      .elapsed_time = Duration32::Seconds(0.3499999940395355),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.28759765625,
      .tilt = Angle::Radians(0.49560546875),
      .orientation = Angle::Radians(0.183349609375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1374.8148193359375, 1093.904541015625},
      .elapsed_time = Duration32::Seconds(0.35199999809265137),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.28466796875,
      .tilt = Angle::Radians(0.49560546875),
      .orientation = Angle::Radians(0.183349609375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1380.1048583984375, 1106.7181396484375},
      .elapsed_time = Duration32::Seconds(0.3540000021457672),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.2822265625,
      .tilt = Angle::Radians(0.49560546875),
      .orientation = Angle::Radians(0.183349609375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1385.24365234375, 1118.34912109375},
      .elapsed_time = Duration32::Seconds(0.35600000619888306),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.27880859375,
      .tilt = Angle::Radians(0.49560546875),
      .orientation = Angle::Radians(0.183349609375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1390.684814453125, 1129.585693359375},
      .elapsed_time = Duration32::Seconds(0.35899999737739563),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.276611328125,
      .tilt = Angle::Radians(0.49560546875),
      .orientation = Angle::Radians(0.183349609375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1396.1259765625, 1139.5408935546875},
      .elapsed_time = Duration32::Seconds(0.3610000014305115),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.275146484375,
      .tilt = Angle::Radians(0.49560546875),
      .orientation = Angle::Radians(0.183349609375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1401.4158935546875, 1148.3133544921875},
      .elapsed_time = Duration32::Seconds(0.3630000054836273),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.275146484375,
      .tilt = Angle::Radians(0.49560546875),
      .orientation = Angle::Radians(0.183349609375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1406.7059326171875, 1156.19873046875},
      .elapsed_time = Duration32::Seconds(0.36500000953674316),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.2734375,
      .tilt = Angle::Radians(0.4931640625),
      .orientation = Angle::Radians(0.1474609375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1411.693603515625, 1163.1968994140625},
      .elapsed_time = Duration32::Seconds(0.367000013589859),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.271484375,
      .tilt = Angle::Radians(0.4931640625),
      .orientation = Angle::Radians(0.1474609375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1416.6812744140625, 1169.603759765625},
      .elapsed_time = Duration32::Seconds(0.36899998784065247),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.269287109375,
      .tilt = Angle::Radians(0.4931640625),
      .orientation = Angle::Radians(0.1474609375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1421.8201904296875, 1175.2220458984375},
      .elapsed_time = Duration32::Seconds(0.3709999918937683),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.265869140625,
      .tilt = Angle::Radians(0.4931640625),
      .orientation = Angle::Radians(0.1474609375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1426.807861328125, 1180.150390625},
      .elapsed_time = Duration32::Seconds(0.37299999594688416),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.26513671875,
      .tilt = Angle::Radians(0.4931640625),
      .orientation = Angle::Radians(0.1474609375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1431.94677734375, 1184.3887939453125},
      .elapsed_time = Duration32::Seconds(0.375),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.264404296875,
      .tilt = Angle::Radians(0.4931640625),
      .orientation = Angle::Radians(0.1474609375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1436.9344482421875, 1187.937255859375},
      .elapsed_time = Duration32::Seconds(0.37700000405311584),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.26318359375,
      .tilt = Angle::Radians(0.475830078125),
      .orientation = Angle::Radians(0.15234375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1441.77099609375, 1190.9927978515625},
      .elapsed_time = Duration32::Seconds(0.3790000081062317),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.261474609375,
      .tilt = Angle::Radians(0.475830078125),
      .orientation = Angle::Radians(0.15234375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1446.3052978515625, 1193.5555419921875},
      .elapsed_time = Duration32::Seconds(0.38100001215934753),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.260009765625,
      .tilt = Angle::Radians(0.473876953125),
      .orientation = Angle::Radians(0.11474609375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1450.6883544921875, 1195.6253662109375},
      .elapsed_time = Duration32::Seconds(0.3840000033378601),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.2587890625,
      .tilt = Angle::Radians(0.473876953125),
      .orientation = Angle::Radians(0.11474609375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1454.769287109375, 1197.1038818359375},
      .elapsed_time = Duration32::Seconds(0.38600000739097595),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.257568359375,
      .tilt = Angle::Radians(0.473876953125),
      .orientation = Angle::Radians(0.11474609375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1458.5478515625, 1198.2867431640625},
      .elapsed_time = Duration32::Seconds(0.3880000114440918),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.2568359375,
      .tilt = Angle::Radians(0.473876953125),
      .orientation = Angle::Radians(0.11474609375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1462.326416015625, 1198.5823974609375},
      .elapsed_time = Duration32::Seconds(0.38999998569488525),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.255126953125,
      .tilt = Angle::Radians(0.473876953125),
      .orientation = Angle::Radians(0.11474609375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1465.802734375, 1198.878173828125},
      .elapsed_time = Duration32::Seconds(0.3919999897480011),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.253173828125,
      .tilt = Angle::Radians(0.473876953125),
      .orientation = Angle::Radians(0.11474609375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1469.1278076171875, 1199.173828125},
      .elapsed_time = Duration32::Seconds(0.39399999380111694),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.251220703125,
      .tilt = Angle::Radians(0.473876953125),
      .orientation = Angle::Radians(0.11474609375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1472.3017578125, 1198.9766845703125},
      .elapsed_time = Duration32::Seconds(0.3959999978542328),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.2490234375,
      .tilt = Angle::Radians(0.472412109375),
      .orientation = Angle::Radians(0.07666015625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1475.32470703125, 1197.7939453125},
      .elapsed_time = Duration32::Seconds(0.39800000190734863),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.247802734375,
      .tilt = Angle::Radians(0.472412109375),
      .orientation = Angle::Radians(0.07666015625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1478.1964111328125, 1196.5125732421875},
      .elapsed_time = Duration32::Seconds(0.4000000059604645),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.247314453125,
      .tilt = Angle::Radians(0.472412109375),
      .orientation = Angle::Radians(0.07666015625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1481.068115234375, 1194.5411376953125},
      .elapsed_time = Duration32::Seconds(0.4020000100135803),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.24609375,
      .tilt = Angle::Radians(0.472412109375),
      .orientation = Angle::Radians(0.07666015625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1483.486328125, 1192.3726806640625},
      .elapsed_time = Duration32::Seconds(0.4039999842643738),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.244140625,
      .tilt = Angle::Radians(0.454833984375),
      .orientation = Angle::Radians(0.079345703125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1486.0557861328125, 1189.5142822265625},
      .elapsed_time = Duration32::Seconds(0.4059999883174896),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.24072265625,
      .tilt = Angle::Radians(0.454833984375),
      .orientation = Angle::Radians(0.079345703125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1488.3228759765625, 1186.1629638671875},
      .elapsed_time = Duration32::Seconds(0.40799999237060547),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.236328125,
      .tilt = Angle::Radians(0.454833984375),
      .orientation = Angle::Radians(0.079345703125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1490.7412109375, 1182.023193359375},
      .elapsed_time = Duration32::Seconds(0.4099999964237213),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.234130859375,
      .tilt = Angle::Radians(0.454833984375),
      .orientation = Angle::Radians(0.079345703125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1492.857177734375, 1177.2919921875},
      .elapsed_time = Duration32::Seconds(0.41200000047683716),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.23291015625,
      .tilt = Angle::Radians(0.454833984375),
      .orientation = Angle::Radians(0.079345703125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1494.822021484375, 1171.6737060546875},
      .elapsed_time = Duration32::Seconds(0.41499999165534973),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.232177734375,
      .tilt = Angle::Radians(0.453857421875),
      .orientation = Angle::Radians(0.03955078125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1496.6357421875, 1165.266845703125},
      .elapsed_time = Duration32::Seconds(0.4169999957084656),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.232666015625,
      .tilt = Angle::Radians(0.453857421875),
      .orientation = Angle::Radians(0.03955078125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1498.147216796875, 1158.1700439453125},
      .elapsed_time = Duration32::Seconds(0.4189999997615814),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.230712890625,
      .tilt = Angle::Radians(0.453857421875),
      .orientation = Angle::Radians(0.03955078125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1499.3563232421875, 1150.0875244140625},
      .elapsed_time = Duration32::Seconds(0.42100000381469727),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.22900390625,
      .tilt = Angle::Radians(0.453857421875),
      .orientation = Angle::Radians(0.03955078125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1499.809814453125, 1141.315185546875},
      .elapsed_time = Duration32::Seconds(0.4230000078678131),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.227294921875,
      .tilt = Angle::Radians(0.453857421875),
      .orientation = Angle::Radians(0.03955078125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1501.0189208984375, 1131.6556396484375},
      .elapsed_time = Duration32::Seconds(0.42500001192092896),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.22314453125,
      .tilt = Angle::Radians(0.453857421875),
      .orientation = Angle::Radians(0.03955078125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1501.3211669921875, 1121.0103759765625},
      .elapsed_time = Duration32::Seconds(0.4269999861717224),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.21875,
      .tilt = Angle::Radians(0.453857421875),
      .orientation = Angle::Radians(0.03955078125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1501.774658203125, 1109.47802734375},
      .elapsed_time = Duration32::Seconds(0.42899999022483826),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.212890625,
      .tilt = Angle::Radians(0.453857421875),
      .orientation = Angle::Radians(0.03955078125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1502.22802734375, 1097.2557373046875},
      .elapsed_time = Duration32::Seconds(0.4309999942779541),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.20654296875,
      .tilt = Angle::Radians(0.4365234375),
      .orientation = Angle::Radians(0.041259765625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1502.6815185546875, 1084.343505859375},
      .elapsed_time = Duration32::Seconds(0.43299999833106995),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.200927734375,
      .tilt = Angle::Radians(0.4365234375),
      .orientation = Angle::Radians(0.041259765625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1502.9837646484375, 1070.4456787109375},
      .elapsed_time = Duration32::Seconds(0.4350000023841858),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.19580078125,
      .tilt = Angle::Radians(0.4365234375),
      .orientation = Angle::Radians(0.041259765625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1502.9837646484375, 1055.956298828125},
      .elapsed_time = Duration32::Seconds(0.43700000643730164),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.1904296875,
      .tilt = Angle::Radians(0.4365234375),
      .orientation = Angle::Radians(0.041259765625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1502.9837646484375, 1040.97412109375},
      .elapsed_time = Duration32::Seconds(0.4390000104904175),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.186279296875,
      .tilt = Angle::Radians(0.4375),
      .orientation = Angle::Radians(0.082275390625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1502.9837646484375, 1025.5977783203125},
      .elapsed_time = Duration32::Seconds(0.44200000166893005),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.18212890625,
      .tilt = Angle::Radians(0.4375),
      .orientation = Angle::Radians(0.082275390625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1503.2860107421875, 1010.1227416992188},
      .elapsed_time = Duration32::Seconds(0.4440000057220459),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.1787109375,
      .tilt = Angle::Radians(0.4375),
      .orientation = Angle::Radians(0.082275390625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1503.58837890625, 994.35205078125},
      .elapsed_time = Duration32::Seconds(0.44600000977516174),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.176513671875,
      .tilt = Angle::Radians(0.4375),
      .orientation = Angle::Radians(0.082275390625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1504.041748046875, 978.5814208984375},
      .elapsed_time = Duration32::Seconds(0.4479999840259552),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.1748046875,
      .tilt = Angle::Radians(0.4375),
      .orientation = Angle::Radians(0.082275390625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1505.4019775390625, 962.8107299804688},
      .elapsed_time = Duration32::Seconds(0.44999998807907104),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.17333984375,
      .tilt = Angle::Radians(0.40283203125),
      .orientation = Angle::Radians(0.0888671875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1505.85546875, 947.2371826171875},
      .elapsed_time = Duration32::Seconds(0.4519999921321869),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.171630859375,
      .tilt = Angle::Radians(0.40283203125),
      .orientation = Angle::Radians(0.0888671875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1507.366943359375, 931.8607177734375},
      .elapsed_time = Duration32::Seconds(0.45399999618530273),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.169677734375,
      .tilt = Angle::Radians(0.40283203125),
      .orientation = Angle::Radians(0.0888671875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1508.7271728515625, 917.17431640625},
      .elapsed_time = Duration32::Seconds(0.4560000002384186),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.168701171875,
      .tilt = Angle::Radians(0.40283203125),
      .orientation = Angle::Radians(0.0888671875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1510.08740234375, 903.1777954101562},
      .elapsed_time = Duration32::Seconds(0.4580000042915344),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.16845703125,
      .tilt = Angle::Radians(0.40283203125),
      .orientation = Angle::Radians(0.0888671875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1511.75, 890.0684204101562},
      .elapsed_time = Duration32::Seconds(0.46000000834465027),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.16796875,
      .tilt = Angle::Radians(0.40283203125),
      .orientation = Angle::Radians(0.0888671875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1513.563720703125, 878.5361328125},
      .elapsed_time = Duration32::Seconds(0.4620000123977661),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.16845703125,
      .tilt = Angle::Radians(0.40283203125),
      .orientation = Angle::Radians(0.0888671875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1515.37744140625, 868.7780151367188},
      .elapsed_time = Duration32::Seconds(0.46399998664855957),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.168701171875,
      .tilt = Angle::Radians(0.404541015625),
      .orientation = Angle::Radians(0.133056640625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1517.34228515625, 860.7941284179688},
      .elapsed_time = Duration32::Seconds(0.4659999907016754),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.16943359375,
      .tilt = Angle::Radians(0.404541015625),
      .orientation = Angle::Radians(0.133056640625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1519.156005859375, 854.6829833984375},
      .elapsed_time = Duration32::Seconds(0.46799999475479126),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.172119140625,
      .tilt = Angle::Radians(0.404541015625),
      .orientation = Angle::Radians(0.133056640625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1520.9697265625, 850.0503540039062},
      .elapsed_time = Duration32::Seconds(0.47099998593330383),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.177490234375,
      .tilt = Angle::Radians(0.404541015625),
      .orientation = Angle::Radians(0.133056640625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1522.63232421875, 847.0933227539062},
      .elapsed_time = Duration32::Seconds(0.4729999899864197),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.18505859375,
      .tilt = Angle::Radians(0.404541015625),
      .orientation = Angle::Radians(0.133056640625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1523.8414306640625, 845.319091796875},
      .elapsed_time = Duration32::Seconds(0.4749999940395355),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.194580078125,
      .tilt = Angle::Radians(0.404541015625),
      .orientation = Angle::Radians(0.133056640625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1525.20166015625, 844.9248657226562},
      .elapsed_time = Duration32::Seconds(0.47699999809265137),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.20361328125,
      .tilt = Angle::Radians(0.404541015625),
      .orientation = Angle::Radians(0.133056640625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1525.6551513671875, 844.9248657226562},
      .elapsed_time = Duration32::Seconds(0.4790000021457672),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.21044921875,
      .tilt = Angle::Radians(0.404541015625),
      .orientation = Angle::Radians(0.133056640625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1527.4688720703125, 845.319091796875},
      .elapsed_time = Duration32::Seconds(0.48100000619888306),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.21630859375,
      .tilt = Angle::Radians(0.404541015625),
      .orientation = Angle::Radians(0.133056640625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1528.677978515625, 847.586181640625},
      .elapsed_time = Duration32::Seconds(0.4830000102519989),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.221435546875,
      .tilt = Angle::Radians(0.406982421875),
      .orientation = Angle::Radians(0.176513671875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1528.980224609375, 850.3460083007812},
      .elapsed_time = Duration32::Seconds(0.48499998450279236),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.227294921875,
      .tilt = Angle::Radians(0.406982421875),
      .orientation = Angle::Radians(0.176513671875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1530.49169921875, 854.1901245117188},
      .elapsed_time = Duration32::Seconds(0.4869999885559082),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.2333984375,
      .tilt = Angle::Radians(0.406982421875),
      .orientation = Angle::Radians(0.176513671875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1530.9451904296875, 859.1184692382812},
      .elapsed_time = Duration32::Seconds(0.48899999260902405),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.23876953125,
      .tilt = Angle::Radians(0.406982421875),
      .orientation = Angle::Radians(0.176513671875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1532.154296875, 865.2296142578125},
      .elapsed_time = Duration32::Seconds(0.4909999966621399),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.243896484375,
      .tilt = Angle::Radians(0.406982421875),
      .orientation = Angle::Radians(0.176513671875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1532.45654296875, 872.6221313476562},
      .elapsed_time = Duration32::Seconds(0.49300000071525574),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.249267578125,
      .tilt = Angle::Radians(0.406982421875),
      .orientation = Angle::Radians(0.176513671875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1532.7587890625, 881.0003051757812},
      .elapsed_time = Duration32::Seconds(0.4950000047683716),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.25390625,
      .tilt = Angle::Radians(0.406982421875),
      .orientation = Angle::Radians(0.176513671875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1533.0611572265625, 890.3641357421875},
      .elapsed_time = Duration32::Seconds(0.4970000088214874),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.25830078125,
      .tilt = Angle::Radians(0.406982421875),
      .orientation = Angle::Radians(0.176513671875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1533.3634033203125, 900.3193969726562},
      .elapsed_time = Duration32::Seconds(0.5),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.26123046875,
      .tilt = Angle::Radians(0.406982421875),
      .orientation = Angle::Radians(0.176513671875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1533.3634033203125, 911.0631713867188},
      .elapsed_time = Duration32::Seconds(0.5019999742507935),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.26171875,
      .tilt = Angle::Radians(0.406982421875),
      .orientation = Angle::Radians(0.176513671875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1533.6656494140625, 922.4968872070312},
      .elapsed_time = Duration32::Seconds(0.5040000081062317),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.26171875,
      .tilt = Angle::Radians(0.406982421875),
      .orientation = Angle::Radians(0.176513671875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1533.6656494140625, 934.1278076171875},
      .elapsed_time = Duration32::Seconds(0.5059999823570251),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.260498046875,
      .tilt = Angle::Radians(0.406982421875),
      .orientation = Angle::Radians(0.176513671875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1534.119140625, 946.0543823242188},
      .elapsed_time = Duration32::Seconds(0.5080000162124634),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.259521484375,
      .tilt = Angle::Radians(0.389892578125),
      .orientation = Angle::Radians(0.18408203125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1534.572509765625, 957.98095703125},
      .elapsed_time = Duration32::Seconds(0.5099999904632568),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.25927734375,
      .tilt = Angle::Radians(0.389892578125),
      .orientation = Angle::Radians(0.18408203125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1535.78173828125, 969.9075317382812},
      .elapsed_time = Duration32::Seconds(0.5120000243186951),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.2587890625,
      .tilt = Angle::Radians(0.389892578125),
      .orientation = Angle::Radians(0.18408203125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1536.083984375, 981.8341064453125},
      .elapsed_time = Duration32::Seconds(0.5139999985694885),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.2587890625,
      .tilt = Angle::Radians(0.389892578125),
      .orientation = Angle::Radians(0.18408203125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1537.4442138671875, 993.3663940429688},
      .elapsed_time = Duration32::Seconds(0.515999972820282),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.25830078125,
      .tilt = Angle::Radians(0.389892578125),
      .orientation = Angle::Radians(0.18408203125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1537.897705078125, 1004.8987426757812},
      .elapsed_time = Duration32::Seconds(0.5180000066757202),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.257568359375,
      .tilt = Angle::Radians(0.389892578125),
      .orientation = Angle::Radians(0.18408203125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1539.2579345703125, 1015.7410888671875},
      .elapsed_time = Duration32::Seconds(0.5199999809265137),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.256591796875,
      .tilt = Angle::Radians(0.389892578125),
      .orientation = Angle::Radians(0.18408203125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1539.71142578125, 1026.38623046875},
      .elapsed_time = Duration32::Seconds(0.5220000147819519),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.255859375,
      .tilt = Angle::Radians(0.389892578125),
      .orientation = Angle::Radians(0.18408203125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1541.2227783203125, 1036.5386962890625},
      .elapsed_time = Duration32::Seconds(0.5239999890327454),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.255615234375,
      .tilt = Angle::Radians(0.389892578125),
      .orientation = Angle::Radians(0.18408203125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1541.67626953125, 1045.705322265625},
      .elapsed_time = Duration32::Seconds(0.5270000100135803),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.25537109375,
      .tilt = Angle::Radians(0.389892578125),
      .orientation = Angle::Radians(0.18408203125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1543.489990234375, 1054.477783203125},
      .elapsed_time = Duration32::Seconds(0.5289999842643738),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.25537109375,
      .tilt = Angle::Radians(0.389892578125),
      .orientation = Angle::Radians(0.18408203125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1543.943359375, 1062.95458984375},
      .elapsed_time = Duration32::Seconds(0.531000018119812),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.255126953125,
      .tilt = Angle::Radians(0.389892578125),
      .orientation = Angle::Radians(0.18408203125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1545.757080078125, 1070.83984375},
      .elapsed_time = Duration32::Seconds(0.5329999923706055),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.25439453125,
      .tilt = Angle::Radians(0.372802734375),
      .orientation = Angle::Radians(0.192138671875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1546.8150634765625, 1078.4295654296875},
      .elapsed_time = Duration32::Seconds(0.5350000262260437),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.254150390625,
      .tilt = Angle::Radians(0.372802734375),
      .orientation = Angle::Radians(0.192138671875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1547.2685546875, 1085.2305908203125},
      .elapsed_time = Duration32::Seconds(0.5370000004768372),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.254638671875,
      .tilt = Angle::Radians(0.372802734375),
      .orientation = Angle::Radians(0.192138671875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1549.2333984375, 1091.341796875},
      .elapsed_time = Duration32::Seconds(0.5389999747276306),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.2548828125,
      .tilt = Angle::Radians(0.372802734375),
      .orientation = Angle::Radians(0.192138671875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1550.5936279296875, 1096.9600830078125},
      .elapsed_time = Duration32::Seconds(0.5410000085830688),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.256103515625,
      .tilt = Angle::Radians(0.372802734375),
      .orientation = Angle::Radians(0.192138671875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1552.1051025390625, 1102.28271484375},
      .elapsed_time = Duration32::Seconds(0.5429999828338623),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.257568359375,
      .tilt = Angle::Radians(0.372802734375),
      .orientation = Angle::Radians(0.192138671875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1553.46533203125, 1107.013916015625},
      .elapsed_time = Duration32::Seconds(0.5450000166893005),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.25830078125,
      .tilt = Angle::Radians(0.372802734375),
      .orientation = Angle::Radians(0.192138671875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1554.976806640625, 1111.2523193359375},
      .elapsed_time = Duration32::Seconds(0.546999990940094),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.259765625,
      .tilt = Angle::Radians(0.372802734375),
      .orientation = Angle::Radians(0.192138671875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1556.639404296875, 1115.29345703125},
      .elapsed_time = Duration32::Seconds(0.5490000247955322),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.260498046875,
      .tilt = Angle::Radians(0.372802734375),
      .orientation = Angle::Radians(0.192138671875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1558.453125, 1118.644775390625},
      .elapsed_time = Duration32::Seconds(0.5509999990463257),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.261474609375,
      .tilt = Angle::Radians(0.372802734375),
      .orientation = Angle::Radians(0.192138671875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1560.1156005859375, 1121.7003173828125},
      .elapsed_time = Duration32::Seconds(0.5540000200271606),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.261962890625,
      .tilt = Angle::Radians(0.372802734375),
      .orientation = Angle::Radians(0.192138671875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1562.231689453125, 1124.6573486328125},
      .elapsed_time = Duration32::Seconds(0.5559999942779541),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.261474609375,
      .tilt = Angle::Radians(0.372802734375),
      .orientation = Angle::Radians(0.192138671875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1564.34765625, 1127.2200927734375},
      .elapsed_time = Duration32::Seconds(0.5580000281333923),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.2607421875,
      .tilt = Angle::Radians(0.372802734375),
      .orientation = Angle::Radians(0.192138671875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1566.463623046875, 1129.6842041015625},
      .elapsed_time = Duration32::Seconds(0.5600000023841858),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.2587890625,
      .tilt = Angle::Radians(0.369873046875),
      .orientation = Angle::Radians(0.144775390625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1568.7308349609375, 1131.9512939453125},
      .elapsed_time = Duration32::Seconds(0.5619999766349792),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.256591796875,
      .tilt = Angle::Radians(0.369873046875),
      .orientation = Angle::Radians(0.144775390625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1570.9979248046875, 1133.8240966796875},
      .elapsed_time = Duration32::Seconds(0.5640000104904175),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.254150390625,
      .tilt = Angle::Radians(0.369873046875),
      .orientation = Angle::Radians(0.144775390625),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1573.416259765625, 1135.401123046875},
      .elapsed_time = Duration32::Seconds(0.5659999847412109),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.251220703125,
      .tilt = Angle::Radians(0.352783203125),
      .orientation = Angle::Radians(0.151611328125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1575.985595703125, 1136.583984375},
      .elapsed_time = Duration32::Seconds(0.5680000185966492),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.248779296875,
      .tilt = Angle::Radians(0.352783203125),
      .orientation = Angle::Radians(0.151611328125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1578.2528076171875, 1137.0767822265625},
      .elapsed_time = Duration32::Seconds(0.5699999928474426),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.2451171875,
      .tilt = Angle::Radians(0.352783203125),
      .orientation = Angle::Radians(0.151611328125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1580.8221435546875, 1138.259521484375},
      .elapsed_time = Duration32::Seconds(0.5720000267028809),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.240966796875,
      .tilt = Angle::Radians(0.352783203125),
      .orientation = Angle::Radians(0.151611328125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1583.08935546875, 1138.259521484375},
      .elapsed_time = Duration32::Seconds(0.5740000009536743),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.23681640625,
      .tilt = Angle::Radians(0.352783203125),
      .orientation = Angle::Radians(0.151611328125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1585.507568359375, 1138.259521484375},
      .elapsed_time = Duration32::Seconds(0.5759999752044678),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.232177734375,
      .tilt = Angle::Radians(0.352783203125),
      .orientation = Angle::Radians(0.151611328125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1587.9259033203125, 1138.0623779296875},
      .elapsed_time = Duration32::Seconds(0.578000009059906),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.228515625,
      .tilt = Angle::Radians(0.352783203125),
      .orientation = Angle::Radians(0.151611328125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1590.495361328125, 1137.668212890625},
      .elapsed_time = Duration32::Seconds(0.5799999833106995),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.224853515625,
      .tilt = Angle::Radians(0.352783203125),
      .orientation = Angle::Radians(0.151611328125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1592.91357421875, 1136.189697265625},
      .elapsed_time = Duration32::Seconds(0.5830000042915344),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.220947265625,
      .tilt = Angle::Radians(0.352783203125),
      .orientation = Angle::Radians(0.151611328125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1595.3319091796875, 1134.8096923828125},
      .elapsed_time = Duration32::Seconds(0.5849999785423279),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.215087890625,
      .tilt = Angle::Radians(0.352783203125),
      .orientation = Angle::Radians(0.151611328125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1597.7501220703125, 1132.93701171875},
      .elapsed_time = Duration32::Seconds(0.5870000123977661),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.20849609375,
      .tilt = Angle::Radians(0.352783203125),
      .orientation = Angle::Radians(0.151611328125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1598.20361328125, 1131.754150390625},
      .elapsed_time = Duration32::Seconds(0.5889999866485596),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.201416015625,
      .tilt = Angle::Radians(0.352783203125),
      .orientation = Angle::Radians(0.151611328125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1600.16845703125, 1129.97998046875},
      .elapsed_time = Duration32::Seconds(0.5910000205039978),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.201171875,
      .tilt = Angle::Radians(0.352783203125),
      .orientation = Angle::Radians(0.151611328125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1601.3775634765625, 1127.9100341796875},
      .elapsed_time = Duration32::Seconds(0.5929999947547913),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.200927734375,
      .tilt = Angle::Radians(0.352783203125),
      .orientation = Angle::Radians(0.151611328125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1603.1912841796875, 1124.9530029296875},
      .elapsed_time = Duration32::Seconds(0.5949999690055847),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.20068359375,
      .tilt = Angle::Radians(0.352783203125),
      .orientation = Angle::Radians(0.151611328125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1604.8538818359375, 1121.601806640625},
      .elapsed_time = Duration32::Seconds(0.597000002861023),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.200439453125,
      .tilt = Angle::Radians(0.352783203125),
      .orientation = Angle::Radians(0.151611328125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1606.6676025390625, 1117.3634033203125},
      .elapsed_time = Duration32::Seconds(0.5989999771118164),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.2001953125,
      .tilt = Angle::Radians(0.352783203125),
      .orientation = Angle::Radians(0.151611328125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1608.6324462890625, 1112.3365478515625},
      .elapsed_time = Duration32::Seconds(0.6010000109672546),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.19970703125,
      .tilt = Angle::Radians(0.352783203125),
      .orientation = Angle::Radians(0.151611328125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1610.8995361328125, 1106.1268310546875},
      .elapsed_time = Duration32::Seconds(0.6029999852180481),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.19921875,
      .tilt = Angle::Radians(0.352783203125),
      .orientation = Angle::Radians(0.151611328125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1613.166748046875, 1099.32568359375},
      .elapsed_time = Duration32::Seconds(0.6050000190734863),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.19873046875,
      .tilt = Angle::Radians(0.33544921875),
      .orientation = Angle::Radians(0.1591796875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1615.433837890625, 1091.5389404296875},
      .elapsed_time = Duration32::Seconds(0.6069999933242798),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.1982421875,
      .tilt = Angle::Radians(0.33544921875),
      .orientation = Angle::Radians(0.1591796875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1617.8521728515625, 1082.963623046875},
      .elapsed_time = Duration32::Seconds(0.609000027179718),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.19775390625,
      .tilt = Angle::Radians(0.33544921875),
      .orientation = Angle::Radians(0.1591796875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1620.1192626953125, 1073.501220703125},
      .elapsed_time = Duration32::Seconds(0.6119999885559082),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.197265625,
      .tilt = Angle::Radians(0.33544921875),
      .orientation = Angle::Radians(0.1591796875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1622.3863525390625, 1063.348876953125},
      .elapsed_time = Duration32::Seconds(0.6140000224113464),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.19677734375,
      .tilt = Angle::Radians(0.33544921875),
      .orientation = Angle::Radians(0.1591796875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1624.653564453125, 1052.309326171875},
      .elapsed_time = Duration32::Seconds(0.6159999966621399),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.1962890625,
      .tilt = Angle::Radians(0.33544921875),
      .orientation = Angle::Radians(0.1591796875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1627.07177734375, 1040.678466796875},
      .elapsed_time = Duration32::Seconds(0.6179999709129333),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.19580078125,
      .tilt = Angle::Radians(0.33544921875),
      .orientation = Angle::Radians(0.1591796875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1629.4901123046875, 1028.259033203125},
      .elapsed_time = Duration32::Seconds(0.6200000047683716),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.195068359375,
      .tilt = Angle::Radians(0.33544921875),
      .orientation = Angle::Radians(0.1591796875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1631.7572021484375, 1015.149658203125},
      .elapsed_time = Duration32::Seconds(0.621999979019165),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.194580078125,
      .tilt = Angle::Radians(0.33544921875),
      .orientation = Angle::Radians(0.1591796875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1634.32666015625, 1001.4489135742188},
      .elapsed_time = Duration32::Seconds(0.6240000128746033),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.19384765625,
      .tilt = Angle::Radians(0.33544921875),
      .orientation = Angle::Radians(0.1591796875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1636.7449951171875, 987.452392578125},
      .elapsed_time = Duration32::Seconds(0.6259999871253967),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.19287109375,
      .tilt = Angle::Radians(0.33544921875),
      .orientation = Angle::Radians(0.1591796875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1639.465576171875, 972.9630737304688},
      .elapsed_time = Duration32::Seconds(0.628000020980835),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.192138671875,
      .tilt = Angle::Radians(0.33544921875),
      .orientation = Angle::Radians(0.1591796875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1642.3372802734375, 958.3751831054688},
      .elapsed_time = Duration32::Seconds(0.6299999952316284),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.19140625,
      .tilt = Angle::Radians(0.33544921875),
      .orientation = Angle::Radians(0.1591796875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1645.51123046875, 943.787353515625},
      .elapsed_time = Duration32::Seconds(0.6319999694824219),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.190673828125,
      .tilt = Angle::Radians(0.33544921875),
      .orientation = Angle::Radians(0.1591796875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1648.5340576171875, 929.6922607421875},
      .elapsed_time = Duration32::Seconds(0.6340000033378601),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.18994140625,
      .tilt = Angle::Radians(0.33544921875),
      .orientation = Angle::Radians(0.1591796875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1652.1614990234375, 915.9915161132812},
      .elapsed_time = Duration32::Seconds(0.6370000243186951),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.189453125,
      .tilt = Angle::Radians(0.33544921875),
      .orientation = Angle::Radians(0.1591796875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1655.6378173828125, 903.2763671875},
      .elapsed_time = Duration32::Seconds(0.6389999985694885),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.18896484375,
      .tilt = Angle::Radians(0.33544921875),
      .orientation = Angle::Radians(0.1591796875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1659.2652587890625, 891.9412231445312},
      .elapsed_time = Duration32::Seconds(0.640999972820282),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.1884765625,
      .tilt = Angle::Radians(0.33544921875),
      .orientation = Angle::Radians(0.1591796875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1662.892578125, 882.18310546875},
      .elapsed_time = Duration32::Seconds(0.6430000066757202),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.188232421875,
      .tilt = Angle::Radians(0.33544921875),
      .orientation = Angle::Radians(0.1591796875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1666.368896484375, 874.1006469726562},
      .elapsed_time = Duration32::Seconds(0.6449999809265137),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.18798828125,
      .tilt = Angle::Radians(0.33544921875),
      .orientation = Angle::Radians(0.1591796875),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1669.84521484375, 867.7923583984375},
      .elapsed_time = Duration32::Seconds(0.6470000147819519),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.18798828125,
      .tilt = Angle::Radians(0.301025390625),
      .orientation = Angle::Radians(0.177001953125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1671.658935546875, 865.5253295898438},
      .elapsed_time = Duration32::Seconds(0.6489999890327454),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.18798828125,
      .tilt = Angle::Radians(0.301025390625),
      .orientation = Angle::Radians(0.177001953125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1673.47265625, 863.3568115234375},
      .elapsed_time = Duration32::Seconds(0.6510000228881836),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.188232421875,
      .tilt = Angle::Radians(0.301025390625),
      .orientation = Angle::Radians(0.177001953125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1675.4375, 861.9769287109375},
      .elapsed_time = Duration32::Seconds(0.652999997138977),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.1884765625,
      .tilt = Angle::Radians(0.301025390625),
      .orientation = Angle::Radians(0.177001953125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1677.553466796875, 860.7941284179688},
      .elapsed_time = Duration32::Seconds(0.6549999713897705),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.188720703125,
      .tilt = Angle::Radians(0.301025390625),
      .orientation = Angle::Radians(0.177001953125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1679.518310546875, 860.7941284179688},
      .elapsed_time = Duration32::Seconds(0.6570000052452087),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.18896484375,
      .tilt = Angle::Radians(0.301025390625),
      .orientation = Angle::Radians(0.177001953125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1681.785400390625, 860.7941284179688},
      .elapsed_time = Duration32::Seconds(0.6589999794960022),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.189208984375,
      .tilt = Angle::Radians(0.301025390625),
      .orientation = Angle::Radians(0.177001953125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1683.9014892578125, 862.2725830078125},
      .elapsed_time = Duration32::Seconds(0.6610000133514404),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.189208984375,
      .tilt = Angle::Radians(0.301025390625),
      .orientation = Angle::Radians(0.177001953125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1686.3197021484375, 864.0468139648438},
      .elapsed_time = Duration32::Seconds(0.6629999876022339),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.189208984375,
      .tilt = Angle::Radians(0.301025390625),
      .orientation = Angle::Radians(0.177001953125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1688.738037109375, 866.7081298828125},
      .elapsed_time = Duration32::Seconds(0.6650000214576721),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.18896484375,
      .tilt = Angle::Radians(0.301025390625),
      .orientation = Angle::Radians(0.177001953125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1691.15625, 869.8622436523438},
      .elapsed_time = Duration32::Seconds(0.6679999828338623),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.188232421875,
      .tilt = Angle::Radians(0.301025390625),
      .orientation = Angle::Radians(0.177001953125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1693.5745849609375, 874.0020751953125},
      .elapsed_time = Duration32::Seconds(0.6700000166893005),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.187255859375,
      .tilt = Angle::Radians(0.301025390625),
      .orientation = Angle::Radians(0.177001953125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1695.9927978515625, 878.6347045898438},
      .elapsed_time = Duration32::Seconds(0.671999990940094),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.18603515625,
      .tilt = Angle::Radians(0.301025390625),
      .orientation = Angle::Radians(0.177001953125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1698.4111328125, 884.1544189453125},
      .elapsed_time = Duration32::Seconds(0.6740000247955322),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.18408203125,
      .tilt = Angle::Radians(0.301025390625),
      .orientation = Angle::Radians(0.177001953125),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1701.1317138671875, 890.0684204101562},
      .elapsed_time = Duration32::Seconds(0.6759999990463257),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.181884765625,
      .tilt = Angle::Radians(0.304443359375),
      .orientation = Angle::Radians(0.234130859375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1703.3988037109375, 896.6724243164062},
      .elapsed_time = Duration32::Seconds(0.6779999732971191),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.179443359375,
      .tilt = Angle::Radians(0.304443359375),
      .orientation = Angle::Radians(0.234130859375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1706.119384765625, 903.8677978515625},
      .elapsed_time = Duration32::Seconds(0.6800000071525574),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.1767578125,
      .tilt = Angle::Radians(0.304443359375),
      .orientation = Angle::Radians(0.234130859375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1708.5377197265625, 911.654541015625},
      .elapsed_time = Duration32::Seconds(0.6819999814033508),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.174072265625,
      .tilt = Angle::Radians(0.304443359375),
      .orientation = Angle::Radians(0.234130859375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1711.1070556640625, 919.9341430664062},
      .elapsed_time = Duration32::Seconds(0.6840000152587891),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.17138671875,
      .tilt = Angle::Radians(0.304443359375),
      .orientation = Angle::Radians(0.234130859375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1713.525390625, 928.5094604492188},
      .elapsed_time = Duration32::Seconds(0.6859999895095825),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.168701171875,
      .tilt = Angle::Radians(0.304443359375),
      .orientation = Angle::Radians(0.234130859375),
  });
  inputs.push_back(StrokeInput{
      .tool_type = StrokeInput::ToolType::kStylus,
      .position = {1713.525390625, 928.5094604492188},
      .elapsed_time = Duration32::Seconds(0.6880000233650208),
      .stroke_unit_length = PhysicalDistance::Centimeters(0.006747095379978418),
      .pressure = 0.168701171875,
      .tilt = Angle::Radians(0.304443359375),
      .orientation = Angle::Radians(0.234130859375),
  });
  auto result = StrokeInputBatch::Create({inputs});
  ABSL_CHECK_OK(result.status());
  return *result;
}  // NOLINT(readability/fn_size)

}  // namespace ink
