// Copyright (c) 2017-2022 Cloudflare, Inc.
// Licensed under the Apache 2.0 license found in the LICENSE file or at:
//     https://opensource.org/licenses/Apache-2.0

#include "sql.h"
#include <sqlite3.h>
#include <stdio.h>

namespace workerd::api {

SqlResult::SqlResult(SqliteDatabase::Query& query) {
  uint columnCount = query.columnCount();

  while (!query.isDone()) {
    kj::Vector<jsg::Dict<SqliteDatabase::Query::ValueOwned>::Field> fields;
    for (uint i = 0; i < columnCount; ++i) {
      fields.add(jsg::Dict<SqliteDatabase::Query::ValueOwned>::Field{
        .name = kj::heapString(query.getColumnName(i)),
        .value = query.getValueOwned(i)
      });
    }
    results.add(jsg::Dict<SqliteDatabase::Query::ValueOwned>{.fields = fields.releaseAsArray()});
    query.nextRow();
  }
}

kj::ArrayPtr<jsg::Dict<SqliteDatabase::Query::ValueOwned>> SqlResult::getResults(jsg::Lock& js) {
  return results;
}

SqlPreparedStatement::SqlPreparedStatement(jsg::Ref<SqlDatabase>&& sqlDb, SqliteDatabase::Statement&& statement):
  sqlDatabase(kj::mv(sqlDb)),
  statement(kj::mv(statement)) {
}

void fillBindValues(
  kj::Vector<SqliteDatabase::Query::ValuePtr>& bindValues, kj::Maybe<kj::Array<ValueBind>>& bindValuesOptional) {
  KJ_IF_MAYBE(values, bindValuesOptional) {
    for (auto& value : *values) {
      KJ_SWITCH_ONEOF(value) {
        KJ_CASE_ONEOF(val, kj::Array<const byte>) {
          bindValues.add(val.asPtr());
        }
        KJ_CASE_ONEOF(val, kj::String) {
          bindValues.add(val.asPtr());
        }
        KJ_CASE_ONEOF(val, double) {
          bindValues.add(val);
        }
      }
    }
  }
}

jsg::Ref<SqlResult> SqlPreparedStatement::run(kj::Maybe<kj::Array<ValueBind>> bindValuesOptional) {
  kj::Vector<SqliteDatabase::Query::ValuePtr> bindValues;
  fillBindValues(bindValues, bindValuesOptional);

  kj::ArrayPtr<const SqliteDatabase::Query::ValuePtr> boundValues = bindValues.asPtr();
  SqliteDatabase::Query query(sqlDatabase->sqlite, statement, boundValues);
  return jsg::alloc<SqlResult>(query);
}

SqlDatabase::SqlDatabase():
  dir(kj::newInMemoryDirectory(kj::nullClock())),
  vfs(*dir),
  sqlite(vfs, kj::Path({"root"}), ErrorCallback([](const char* errStr) {
    JSG_ASSERT(false, Error, errStr);
  }), kj::WriteMode::CREATE | kj::WriteMode::MODIFY) {
  sqlite3* db = sqlite;
  sqlite3_set_authorizer(db, authorize, this);
}

const char* getAuthorizorTableName(int actionCode, const char* param1, const char* param2) {
  switch (actionCode) {
    case SQLITE_CREATE_INDEX:        return param2;
    case SQLITE_CREATE_TABLE:        return param1;
    case SQLITE_CREATE_TEMP_INDEX:   return param2;
    case SQLITE_CREATE_TEMP_TABLE:   return param1;
    case SQLITE_CREATE_TEMP_TRIGGER: return param2;
    case SQLITE_CREATE_TRIGGER:      return param2;
    case SQLITE_DELETE:              return param1;
    case SQLITE_DROP_INDEX:          return param2;
    case SQLITE_DROP_TABLE:          return param1;
    case SQLITE_DROP_TEMP_INDEX:     return param2;
    case SQLITE_DROP_TEMP_TABLE:     return param1;
    case SQLITE_DROP_TEMP_TRIGGER:   return param2;
    case SQLITE_DROP_TRIGGER:        return param2;
    case SQLITE_INSERT:              return param1;
    case SQLITE_READ:                return param1;
    case SQLITE_UPDATE:              return param1;
    case SQLITE_ALTER_TABLE:         return param2;
    case SQLITE_ANALYZE:             return param1;
    case SQLITE_CREATE_VTABLE:       return param1;
    case SQLITE_DROP_VTABLE:         return param1;
  }
  return "";
}

int SqlDatabase::authorize(
  void* userdata,
  int actionCode,
  const char* param1,
  const char* param2,
  const char* dbName,
  const char* triggerName) {
  SqlDatabase* self = reinterpret_cast<SqlDatabase*>(userdata);
  fprintf(stderr, "AUTHORIZE CALLBACK isAdmin(%d) '%d' '%s' '%s' '%s' '%s'\n", int(self->isAdmin), actionCode, param1, param2, dbName, triggerName);

  if (self->isAdmin) {
    return SQLITE_OK;
  }

  kj::StringPtr tableName = getAuthorizorTableName(actionCode, param1, param2);
  if (tableName.startsWith("_cf_")) {
    fprintf(stderr, "USER ATTEMPTED _cf_ TABLE ACCESS\n");
    return SQLITE_DENY;
  }

  if (actionCode == SQLITE_PRAGMA) {
    // Implement allow list here
    fprintf(stderr, "USER ATTEMPTED PRAGMA\n");
    return SQLITE_DENY;
  }

  if (actionCode == SQLITE_TRANSACTION) {
    fprintf(stderr, "USER ATTEMPTED TRANSACTION\n");
    return SQLITE_DENY;
  }
  return SQLITE_OK;
}

jsg::Ref<SqlDatabase> SqlDatabase::constructor(jsg::Lock& js, CompatibilityFlags::Reader flags) {
  return jsg::alloc<SqlDatabase>();
}

jsg::Ref<SqlResult> SqlDatabase::exec(jsg::Lock& js, kj::String querySql, bool admin, kj::Maybe<kj::Array<ValueBind>> bindValuesOptional) {
  kj::Vector<SqliteDatabase::Query::ValuePtr> bindValues;
  fillBindValues(bindValues, bindValuesOptional);

  kj::String error;
  kj::ArrayPtr<const SqliteDatabase::Query::ValuePtr> boundValues = bindValues.asPtr();
  isAdmin = admin;
  ErrorCallback errorCb = ErrorCallback([](const char* errStr) {
    JSG_ASSERT(false, Error, errStr);
  });
  SqliteDatabase::Query query(sqlite, querySql, errorCb, boundValues);
  isAdmin = false;
  return jsg::alloc<SqlResult>(query);
}

jsg::Ref<SqlPreparedStatement> SqlDatabase::prepare(jsg::Lock& js, kj::String query, bool admin) {
  isAdmin = admin;
  SqliteDatabase::Statement statement = sqlite.prepare(query);
  isAdmin = false;
  return jsg::alloc<SqlPreparedStatement>(JSG_THIS, kj::mv(statement));
}

}  // namespace workerd::api
