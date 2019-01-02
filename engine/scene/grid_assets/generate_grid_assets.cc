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

#include <string>
#include <vector>

#include "base/commandlineflags.h"
#include "base/init_google.h"
#include "base/logging.h"
#include "file/base/filesystem.h"
#include "file/base/helpers.h"
#include "file/base/options.h"
#include "file/base/path.h"
#include "third_party/absl/memory/memory.h"
#include "third_party/absl/strings/str_cat.h"
#include "third_party/absl/strings/string_view.h"
#include "third_party/absl/strings/substitute.h"
#include "ink/engine/public/types/client_bitmap.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"
#include "ink/offscreen/pix.h"
#include "ink/proto/bitmap_portable_proto.pb.h"
#include "third_party/zlib/zlib.h"
#include "util/task/status.h"

// This program is used to package PNG grid assets as RGBA8888 zipped raw bitmap
// data in a cc_embed_data rule for use in grid_manager.cc.
//
// See the grid_assets_proto genrule for invocation details.
//
// The zipped raw format is used in place of embedding the PNGs themselves to
// avoid adding PNG decoding library to the binary, and the zipped format gives
// good results for typical grid assets which are mostly blank.

namespace {

constexpr char kGridAssetPath[] =
    "ink/engine/scene/grid_assets/";

DEFINE_string(output_dir, "", "Root directory to write golden images.");

// Compress the pixel data in the given Pix into the out vector.  The out vector
// is resized to the actual amount of data written.
util::Status CompressBitmap(const ink::Pix& grid_image,
                            std::vector<uint8_t>* out) {
  size_t n_bytes =
      grid_image.width() * grid_image.height() *
      ink::bytesPerTexelForFormat(ink::ImageFormat::BITMAP_FORMAT_RGBA_8888);
  size_t n_zip_bytes = n_bytes;
  out->resize(n_bytes);
  int result =
      compress(out->data(), &n_zip_bytes,
               reinterpret_cast<const Bytef*>(grid_image.pixels()), n_bytes);
  out->resize(n_zip_bytes);
  if (result != Z_OK) {
    return util::Status(absl::StatusCode::kUnknown,
                        absl::Substitute("zlib error $0", result));
  }
  auto compression_ratio =
      (1.0f - static_cast<float>(n_zip_bytes) / n_bytes) * 100;
  LOG(INFO) << "Compressed by " << compression_ratio << "\n";
  return util::OkStatus();
}

// Decompress the compressed_bitmap_blob field in the given bitmap proto into
// the out vector.  The out vector is resized to the actual size of the
// decompressed image data.
util::Status UncompressBitmap(const ink::proto::Bitmap& test_bitmap,
                              std::vector<uint8_t>* out) {
  size_t n_bytes =
      test_bitmap.width() * test_bitmap.height() *
      bytesPerTexelForFormat(ink::ImageFormat::BITMAP_FORMAT_RGBA_8888);
  out->resize(n_bytes);
  int zres = uncompress(out->data(), &n_bytes,
                        reinterpret_cast<const Bytef*>(
                            test_bitmap.compressed_bitmap_blob().data()),
                        test_bitmap.compressed_bitmap_blob().size());
  out->resize(n_bytes);
  if (zres != Z_OK) {
    return util::Status(absl::StatusCode::kUnknown,
                        absl::Substitute("zlib error $0", zres));
  }
  return util::OkStatus();
}

// Returns OkStatus if the raw bitmap proto file with the given filename matches
// the given pix.  This is used to ensure that the bitmaps round trip correctly.
util::Status CheckBitmapEquals(std::string proto_name,
                               const ink::Pix& grid_image) {
  ::string serialized_proto;
  CHECK_OK(file::GetContents(proto_name, &serialized_proto, file::Defaults()));
  ink::proto::Bitmap test_bitmap;
  if (!test_bitmap.ParseFromString(serialized_proto)) {
    return util::Status(absl::StatusCode::kInvalidArgument,
                        absl::Substitute("Parsing failed for $0", proto_name));
  }

  std::vector<uint8_t> bitmap_bytes;
  CHECK_OK(UncompressBitmap(test_bitmap, &bitmap_bytes));

  std::unique_ptr<ink::Pix> test_pix;
  CHECK_OK(ink::Pix::fromRgba(bitmap_bytes.data(), test_bitmap.width(),
                              test_bitmap.height(),
                              ink::Pix::AlphaType::PREMULTIPLIED, &test_pix));

  if (!test_pix->equals(grid_image)) {
    return util::Status(absl::StatusCode::kUnknown,
                        "Decoded grid image doesn't equal original!");
  }
  return util::OkStatus();
}

}  // namespace

namespace ink {

void exit() { std::exit(1); }

}  // namespace ink

// Process all PNGs found at kGridAssetPath writing them out as zipped raw
// bitmap proto files in the specified output_dir.
int main(int argc, char* argv[]) {
  InitGoogle(argv[0], &argc, &argv, true);

  std::vector<::string> grid_png_paths;
  CHECK_OK(file::Match(file::JoinPath(kGridAssetPath, "*.png"), &grid_png_paths,
                       file::Defaults()))
      << "No files found!";

  LOG(INFO) << "Found " << grid_png_paths.size() << " files.\n";

  std::unique_ptr<ink::Pix> grid_image;
  for (const auto& grid_png_path : grid_png_paths) {
    CHECK_OK(ink::Pix::fromFile(grid_png_path, &grid_image));
    // Ink textures must be premultiplied.
    grid_image = grid_image->premultiplied();

    ink::proto::Bitmap bitmap_proto;
    bitmap_proto.set_width(grid_image->width());
    bitmap_proto.set_height(grid_image->height());

    std::vector<uint8_t> zip_bytes;
    CHECK_OK(CompressBitmap(*grid_image, &zip_bytes));
    bitmap_proto.set_compressed_bitmap_blob(zip_bytes.data(), zip_bytes.size());

    auto proto_name = file::JoinPath(
        FLAGS_output_dir, absl::StrCat(file::Stem(grid_png_path), ".rawproto"));
    CHECK_OK(file::SetContents(proto_name, bitmap_proto.SerializeAsString(),
                               file::Overwrite()));

    CHECK_OK(CheckBitmapEquals(proto_name, *grid_image));
  }

  LOG(INFO) << "Grid assets updated!\n";
  return 0;
}
