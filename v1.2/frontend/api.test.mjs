import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import vm from "node:vm";

function loadApi(fakeFetch) {
  const context = {
    console,
    AbortController,
    clearTimeout,
    setTimeout,
    URL,
    URLSearchParams,
    window: {},
    fetch: fakeFetch,
  };
  context.globalThis = context;
  vm.createContext(context);
  vm.runInContext(readFileSync(new URL("./js/api.js", import.meta.url), "utf8"), context);
  return context.window.StationeryApi;
}

const calls = [];
const api = loadApi(async (url, options) => {
  calls.push({ url, options });
  return {
    ok: true,
    status: 200,
    text: async () => '{"message":"System stopped"}',
  };
});

const result = await api.shutdown();

assert.equal(result.message, "System stopped");
assert.equal(calls.length, 1);
assert.equal(calls[0].url, "http://localhost:8080/api/system/shutdown");
assert.equal(calls[0].options.method, "POST");

console.log("api shutdown test passed");
