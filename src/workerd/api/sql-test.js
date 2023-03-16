function assertEqual(a, b) {
  if (a !== b) {
    throw new Error(a + " !== " + b);
  }
}

function requireException(callback, expectStr) {
  try {
    callback();
  } catch (err) {
    const errStr = `${err}`;
    if (!errStr.includes(expectStr)) {
      throw new Error(`Got unexpected exception '${errStr}', expected: '${expectStr}'`);
    }
    return;
  }
  throw new Error(`Expected exception '${expectStr}' but none was thrown`);
}

export default {
  async test(ctrl, env, ctx) {

    const sql = new SqlDatabase();

    // Test numeric results
    const resultNumber = sql.exec("SELECT 123", null).results;
    assertEqual(resultNumber.length, 1);
    assertEqual(resultNumber[0]["123"], 123);

    // Test string results
    const resultStr = sql.exec("SELECT 'hello'", null).results;
    assertEqual(resultStr.length, 1);
    assertEqual(resultStr[0]["'hello'"], "hello");

    // Test blob results
    const resultBlob = sql.exec("SELECT x'ff'", null).results;
    assertEqual(resultBlob.length, 1);
    const blob = resultBlob[0]["x'ff'"];
    assertEqual(blob.length, 1);
    assertEqual(blob[0], 255);

    // Test binding values
    const resultBinding = sql.exec("SELECT ?", [456]).results;
    assertEqual(resultBinding.length, 1);
    assertEqual(resultBinding[0]["?"], 456);

    // Large integers
    requireException(() => sql.exec("SELECT 9007199254740992", null),
      "Integers larger than MAX_SAFE_INTEGER must be converted to string");

    // Empty statements
    requireException(() => sql.exec("", null),
      "SQL code did not contain a statement");
    requireException(() => sql.exec(";", null),
      "SQL code did not contain a statement");

    // Invalid statements
    requireException(() => sql.exec("SELECT ;", null),
      "syntax error");

    // Incorrect number of binding values
    requireException(() => sql.exec("SELECT ?", null),
      "wrong number of bindings for SQLite query");

    // Prepared statement
    const prepared = sql.prepare("SELECT 789");
    const resultPrepared = prepared.run(null).results;
    assertEqual(resultPrepared.length, 1);
    assertEqual(resultPrepared[0]["789"], 789);

    // Prepared statement with binding values
    const preparedWithBinding = sql.prepare("SELECT ?");
    const resultPreparedWithBinding = preparedWithBinding.run([789]).results;
    assertEqual(resultPreparedWithBinding.length, 1);
    assertEqual(resultPreparedWithBinding[0]["?"], 789);

    // Prepared statement (incorrect number of binding values)
    requireException(() => preparedWithBinding.run([]),
      "wrong number of bindings for SQLite query");
  }
}
