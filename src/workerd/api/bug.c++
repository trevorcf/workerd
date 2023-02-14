// Copyright (c) 2017-2022 Cloudflare, Inc.
// Licensed under the Apache 2.0 license found in the LICENSE file or at:
//     https://opensource.org/licenses/Apache-2.0

#include "bug.h"

namespace workerd::api {

Bug::Bug() {
  results.add(BugPrimitiveType(123));
}

jsg::Ref<Bug> Bug::constructor(jsg::Lock& js, CompatibilityFlags::Reader flags) {
  return jsg::alloc<Bug>();
}

kj::ArrayPtr<BugPrimitiveType> Bug::getResults(jsg::Lock& js) {
  return results;
}

}  // namespace workerd::api
