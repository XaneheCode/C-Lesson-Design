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
  vm.runInContext(readFileSync(new URL("./api.js", import.meta.url), "utf8"), context);
  return context.window.StationeryApi;
}

const plain = (value) => JSON.parse(JSON.stringify(value));

{
  const api = loadApi(async () => ({
    ok: true,
    status: 200,
    text: async () => "{}",
  }));

  assert.deepEqual(plain(api.normalizeProduct({
    id: "P001",
    name: "晨光中性笔",
    category: "笔",
    manufacturer: "晨光",
    model: "K35",
    stock: "12",
    price: "2.5",
  })), {
    id: "P001",
    name: "晨光中性笔",
    category: "笔",
    maker: "晨光",
    model: "K35",
    stock: 12,
    price: 2.5,
  });

  assert.deepEqual(plain(api.denormalizeProduct({
    id: "ignored",
    name: "新品",
    category: "本",
    maker: "国誉",
    model: "A5",
    stock: 0,
    price: 12,
    __mode: "create",
  })), {
    name: "新品",
    category: "本",
    manufacturer: "国誉",
    model: "A5",
    stock: 0,
    price: 12,
  });
}

{
  const api = loadApi(async () => ({
    ok: true,
    status: 200,
    text: async () => "{}",
  }));

  assert.deepEqual(plain(api.normalizeRecord({
    id: "S001",
    product_id: "P001",
    name: "晨光中性笔",
    category: "笔",
    date: "2026-05-28",
    quantity: "3",
    price: "2",
    total: "6",
  })), {
    id: "S001",
    productId: "P001",
    name: "晨光中性笔",
    category: "笔",
    date: "2026-05-28",
    qty: 3,
    price: 2,
    total: 6,
  });

  assert.deepEqual(plain(api.denormalizeRecord({
    id: "ignored",
    productId: "P001",
    date: "2026-05-28",
    qty: 3,
    price: 2,
  })), {
    product_id: "P001",
    date: "2026-05-28",
    quantity: 3,
    price: 2,
  });
}

{
  const api = loadApi(async () => ({
    ok: true,
    status: 200,
    text: async () => '{"error":"库存不足"}',
  }));
  await assert.rejects(
    () => api.request("/sales", { method: "POST" }),
    /库存不足/
  );
}

{
  const api = loadApi(async () => ({
    ok: false,
    status: 500,
    text: async () => '{"message":"server exploded"}',
  }));
  await assert.rejects(
    () => api.request("/products"),
    /server exploded/
  );
}

{
  const api = loadApi(async () => ({
    ok: true,
    status: 200,
    text: async () => "{not json",
  }));
  await assert.rejects(
    () => api.request("/products"),
    /无法解析服务器响应/
  );
}

console.log("api tests passed");
