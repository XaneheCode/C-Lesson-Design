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

Object.assign(window, {
  CATEGORIES, INITIAL_INVENTORY,
  TODAY, today
});
