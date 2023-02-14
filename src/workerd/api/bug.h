// Copyright (c) 2017-2022 Cloudflare, Inc.
// Licensed under the Apache 2.0 license found in the LICENSE file or at:
//     https://opensource.org/licenses/Apache-2.0

#pragma once

#include <workerd/jsg/jsg.h>
#include "basics.h"

namespace workerd::api {

// Comment one or the other out to see the bug behvaior
// int64_t causes something weird, and int32_t works fine
typedef int64_t BugPrimitiveType;
//typedef int32_t BugPrimitiveType;

class Bug final: public jsg::Object {
public:
  Bug();

  static jsg::Ref<Bug> constructor(jsg::Lock& js, CompatibilityFlags::Reader flags);

  kj::ArrayPtr<BugPrimitiveType> getResults(jsg::Lock& js);

  JSG_RESOURCE_TYPE(Bug, CompatibilityFlags::Reader flags) {
    JSG_READONLY_PROTOTYPE_PROPERTY(results, getResults);
  }

private:
  kj::Vector<BugPrimitiveType> results;
};

#define EW_BUG_ISOLATE_TYPES       \
  api::Bug

}  // namespace workerd::api
