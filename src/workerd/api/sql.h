// Copyright (c) 2017-2022 Cloudflare, Inc.
// Licensed under the Apache 2.0 license found in the LICENSE file or at:
//     https://opensource.org/licenses/Apache-2.0

#pragma once

#include <workerd/jsg/jsg.h>
#include <workerd/jsg/string.h>
#include "basics.h"

namespace workerd::api {

typedef kj::OneOf<kj::Array<const byte>, kj::String, double> ValueBind;
class SqlDatabase;

class SqlResult final: public jsg::Object {
public:
  SqlResult(SqliteDatabase::Query& query);

  kj::ArrayPtr<jsg::Dict<SqliteDatabase::Query::ValueOwned>> getResults(jsg::Lock& js);

  JSG_RESOURCE_TYPE(SqlResult, CompatibilityFlags::Reader flags) {
    JSG_READONLY_PROTOTYPE_PROPERTY(results, getResults);
  }

private:
  kj::Vector<jsg::Dict<SqliteDatabase::Query::ValueOwned>> results;
};

class SqlPreparedStatement final: public jsg::Object {
public:
  SqlPreparedStatement(jsg::Ref<SqlDatabase>&& sqlDb, SqliteDatabase::Statement&& statement);

  jsg::Ref<SqlResult> run(kj::Maybe<kj::Array<ValueBind>> bindValuesOptional);

  JSG_RESOURCE_TYPE(SqlPreparedStatement, CompatibilityFlags::Reader flags) {
    JSG_METHOD(run);
  }

private:
  void visitForGc(jsg::GcVisitor& visitor) {
    visitor.visit(sqlDatabase);
  }

  jsg::Ref<SqlDatabase> sqlDatabase;
  SqliteDatabase::Statement statement;
};

class SqlDatabase final: public jsg::Object {
public:
  friend class SqlPreparedStatement;
  SqlDatabase();

  static jsg::Ref<SqlDatabase> constructor(jsg::Lock& js, CompatibilityFlags::Reader flags);
  // Creates a new SqlDatabase.

  jsg::Ref<SqlResult> exec(jsg::Lock& js, kj::String query, bool admin, kj::Maybe<kj::Array<ValueBind>> bindValuesOptional);
  jsg::Ref<SqlPreparedStatement> prepare(jsg::Lock& js, kj::String query, bool admin);

  JSG_RESOURCE_TYPE(SqlDatabase, CompatibilityFlags::Reader flags) {
    JSG_METHOD(exec);
    JSG_METHOD(prepare);
  }

private:
  static int authorize(
    void* userdata,
    int actionCode,
    const char* param1,
    const char* param2,
    const char* dbName,
    const char* triggerName);

  void onError(const char* errStr);

  kj::Own<kj::Directory> dir;
  SqliteDatabase::Vfs vfs;
  SqliteDatabase sqlite;

  bool isAdmin = false;
};

#define EW_SQL_ISOLATE_TYPES       \
  api::SqlDatabase,                \
  api::SqlPreparedStatement,       \
  api::SqlResult
// The list of sql.h types that are added to worker.c++'s JSG_DECLARE_ISOLATE_TYPE

}  // namespace workerd::api
