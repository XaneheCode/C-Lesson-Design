/* ============================================
   文房 · Mock Data
   24 库存 / 26 交易 / 14 进货
   ============================================ */

const CATEGORIES = ["笔", "本", "尺", "橡皮", "其他"];

// Use the browser's local calendar date for API submissions and date filters.
const TODAY = new Date();
const formatLocalDate = (d) => {
  const y = d.getFullYear();
  const m = String(d.getMonth() + 1).padStart(2, "0");
  const day = String(d.getDate()).padStart(2, "0");
  return `${y}-${m}-${day}`;
};
const today = (offsetDays = 0) => {
  const d = new Date(TODAY);
  d.setDate(d.getDate() + offsetDays);
  return formatLocalDate(d);
};

/* ---------- 库存 / Inventory (24 items) ---------- */
const INITIAL_INVENTORY = [
  { id: "WJ-0101", name: "晨光中性笔 K35", category: "笔",   maker: "晨光",     model: "0.5mm 黑", stock: 248, price:  2.00 },
  { id: "WJ-0102", name: "真彩中性笔 0009", category: "笔",   maker: "真彩",     model: "0.5mm 蓝", stock: 186, price:  1.80 },
  { id: "WJ-0103", name: "三菱 UM-100",     category: "笔",   maker: "三菱",     model: "0.5mm 黑", stock:  64, price: 12.00 },
  { id: "WJ-0104", name: "派克威雅钢笔",     category: "笔",   maker: "派克",     model: "F尖 黑杆",  stock:  18, price:298.00 },
  { id: "WJ-0105", name: "英雄 329 钢笔",   category: "笔",   maker: "英雄",     model: "EF 蓝杆",  stock:  42, price: 35.00 },
  { id: "WJ-0106", name: "施德楼 2B 铅笔",   category: "笔",   maker: "施德楼",   model: "2B",       stock: 320, price:  3.50 },
  { id: "WJ-0107", name: "中华 6151 铅笔",   category: "笔",   maker: "中华",     model: "HB",       stock: 412, price:  1.20 },
  { id: "WJ-0108", name: "晨光自动铅笔",     category: "笔",   maker: "晨光",     model: "0.5mm",    stock: 102, price:  6.50 },
  { id: "WJ-0109", name: "辉柏嘉彩铅 12色",  category: "笔",   maker: "辉柏嘉",   model: "12色装",   stock:  36, price: 48.00 },
  { id: "WJ-0110", name: "斑马荧光笔",       category: "笔",   maker: "斑马",     model: "黄色",     stock:  88, price:  8.00 },

  { id: "WJ-0201", name: "国誉无线装订本",   category: "本",   maker: "国誉",     model: "A5 80页", stock:  74, price: 18.00 },
  { id: "WJ-0202", name: "得力软抄本",       category: "本",   maker: "得力",     model: "B5 60页", stock: 120, price:  5.00 },
  { id: "WJ-0203", name: "Moleskine 经典",  category: "本",   maker: "Moleskine",model: "Pocket",   stock:   9, price:148.00 },
  { id: "WJ-0204", name: "申士错题本",       category: "本",   maker: "申士",     model: "A4",       stock:  56, price: 12.50 },
  { id: "WJ-0205", name: "九口山牛皮本",     category: "本",   maker: "九口山",   model: "A5 横线", stock:  34, price: 22.00 },

  { id: "WJ-0301", name: "得力直尺",         category: "尺",   maker: "得力",     model: "20cm",     stock: 156, price:  2.50 },
  { id: "WJ-0302", name: "晨光三角尺套装",   category: "尺",   maker: "晨光",     model: "2件套",    stock:  78, price:  6.00 },
  { id: "WJ-0303", name: "真彩量角器",       category: "尺",   maker: "真彩",     model: "180°",     stock:  92, price:  2.00 },

  { id: "WJ-0401", name: "樱花绘图橡皮",     category: "橡皮", maker: "樱花",     model: "小号",     stock: 142, price:  3.50 },
  { id: "WJ-0402", name: "辉柏嘉橡皮",       category: "橡皮", maker: "辉柏嘉",   model: "大号",     stock:  68, price:  6.00 },
  { id: "WJ-0403", name: "晨光 4B 橡皮",     category: "橡皮", maker: "晨光",     model: "4B",       stock: 210, price:  1.50 },
  { id: "WJ-0404", name: "国誉无屑橡皮",     category: "橡皮", maker: "国誉",     model: "标准",     stock:  52, price:  9.00 },
  { id: "WJ-0405", name: "派通 Ain 橡皮",    category: "橡皮", maker: "派通",     model: "ZEAH06",   stock:  44, price:  8.00 },

  { id: "WJ-0501", name: "晨光修正带",       category: "其他", maker: "晨光",     model: "5m",       stock:  96, price:  4.50 },
  { id: "WJ-0502", name: "齐心文件夹",       category: "其他", maker: "齐心",     model: "A4",       stock:  72, price: 12.00 },
  { id: "WJ-0503", name: "3M 便利贴",        category: "其他", maker: "3M",       model: "76×76",    stock: 134, price:  8.50 },
];

/* ---------- 交易 / Transactions (28 items) ---------- */
// Within last ~14 days, with rich eraser sales in last 7 days for the stats
const T = (id, idx, daysAgo, qty, price) => {
  const item = INITIAL_INVENTORY.find(i => i.id === idx);
  return {
    id, productId: idx,
    name: item.name, category: item.category,
    date: today(-daysAgo),
    qty, price,
  };
};

const INITIAL_TRANSACTIONS = [
  T("T-2401", "WJ-0101", 0,  24,  2.00),
  T("T-2402", "WJ-0401", 0,   8,  3.50),
  T("T-2403", "WJ-0202", 1,  15,  5.00),
  T("T-2404", "WJ-0403", 1,  22,  1.50),
  T("T-2405", "WJ-0301", 1,  10,  2.50),
  T("T-2406", "WJ-0107", 2,  48,  1.20),
  T("T-2407", "WJ-0402", 2,   6,  6.00),
  T("T-2408", "WJ-0501", 2,   4,  4.50),
  T("T-2409", "WJ-0204", 3,  12, 12.50),
  T("T-2410", "WJ-0405", 3,   5,  8.00),
  T("T-2411", "WJ-0103", 3,   3, 12.00),
  T("T-2412", "WJ-0401", 4,  12,  3.50),
  T("T-2413", "WJ-0302", 4,   8,  6.00),
  T("T-2414", "WJ-0106", 4,  30,  3.50),
  T("T-2415", "WJ-0404", 5,   6,  9.00),
  T("T-2416", "WJ-0203", 5,   2,148.00),
  T("T-2417", "WJ-0102", 5,  18,  1.80),
  T("T-2418", "WJ-0403", 6,  16,  1.50),
  T("T-2419", "WJ-0503", 6,  10,  8.50),
  T("T-2420", "WJ-0303", 6,   8,  2.00),
  T("T-2421", "WJ-0108", 7,   4,  6.50),
  T("T-2422", "WJ-0109", 8,   2, 48.00),
  T("T-2423", "WJ-0401", 8,   6,  3.50),
  T("T-2424", "WJ-0105", 9,   1, 35.00),
  T("T-2425", "WJ-0205", 10,  3, 22.00),
  T("T-2426", "WJ-0110", 11,  5,  8.00),
  T("T-2427", "WJ-0502", 12,  4, 12.00),
  T("T-2428", "WJ-0201", 13,  2, 18.00),
];

/* ---------- 进货 / Purchases (14 items) ---------- */
const P = (id, idx, daysAgo, qty, price) => {
  const item = INITIAL_INVENTORY.find(i => i.id === idx);
  return {
    id, productId: idx,
    name: item.name, category: item.category,
    date: today(-daysAgo),
    qty, price,
  };
};
const INITIAL_PURCHASES = [
  P("P-1801", "WJ-0101", 2,  200, 1.40),
  P("P-1802", "WJ-0401", 3,  100, 2.20),
  P("P-1803", "WJ-0403", 3,  150, 0.90),
  P("P-1804", "WJ-0107", 5,  500, 0.65),
  P("P-1805", "WJ-0202", 6,  100, 3.20),
  P("P-1806", "WJ-0301", 8,  150, 1.40),
  P("P-1807", "WJ-0402", 10,  60, 3.80),
  P("P-1808", "WJ-0204", 12,  60, 7.50),
  P("P-1809", "WJ-0503", 14, 100, 5.20),
  P("P-1810", "WJ-0106", 18, 300, 1.80),
  P("P-1811", "WJ-0102", 22, 200, 1.10),
  P("P-1812", "WJ-0501", 25, 100, 2.80),
  P("P-1813", "WJ-0404", 28,  60, 5.50),
  P("P-1814", "WJ-0405", 33,  50, 5.00),
];

Object.assign(window, {
  CATEGORIES, INITIAL_INVENTORY, INITIAL_TRANSACTIONS, INITIAL_PURCHASES,
  TODAY, today
});
