// Copyright 2026 Google LLC
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

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

/**
 * Protocol for a callback to allow the caller to provide a particular UIImage corresponding to a
 * client-provided texture ID.
 */
@protocol INKTextureImageSource <NSObject>

/**
 * Retrieve a UIImage for the given texture ID. This may be called synchronously during drawing,
 * so preferably this should use pre-loaded UIImage objects (that is, initialized from CGImage
 * instead of CIImage).
 *
 * Textures can be disabled by having load always return null. null should also be returned when a
 * texture can not be loaded. If null is returned, the texture layer in question should be
 * ignored, allowing for graceful fallback. It's recommended that implementations log when a
 * texture can not be loaded.
 *
 * @param textureID The client-provided texture ID.
 * @return The texture image, if any, associated with the given ID.
 */
- (nullable UIImage *)textureForID:(NSString *)textureID;

@optional

/**
 * Returns the texture ID to write in place of `textureID` when a brush family is encoded with this
 * texture source, for example by `INKBrushFamilyStorage`. If this method is not implemented,
 * texture IDs are written unchanged. This is not used for rendering.
 *
 * Implement this to undo any change made to texture IDs when decoding, for example to remove a
 * prefix that was added to keep texture IDs unique within this texture source, so that re-encoding
 * a decoded brush family writes the texture IDs that it was originally serialized with.
 *
 * This is called for each texture ID used by the brush family being encoded, whether or not
 * `textureForID:` returns an image for it, and it may be called more than once for the same ID. It
 * should be deterministic, and it should return distinct IDs for distinct textures; duplicate IDs
 * are not detected or resolved. If the brush family was decoded from data that also contained
 * fallback versions of it for other versions of Ink, that data, which already has the original
 * texture IDs, is written instead, and this method is not called.
 *
 * @param textureID The client-provided texture ID used by the brush family being encoded.
 * @return The texture ID to write into the serialized brush family.
 */
- (NSString *)encodedTextureIDForID:(NSString *)textureID;

@end

NS_ASSUME_NONNULL_END
