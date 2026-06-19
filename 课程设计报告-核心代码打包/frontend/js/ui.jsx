/* ============================================
   文房 · Shared UI primitives
   ============================================ */
const { useState, useEffect, useMemo, useRef, useCallback } = React;

/* Format helpers */
const fmtMoney = (n) => "¥" + Number(n).toFixed(2);
const fmtInt   = (n) => Number(n).toLocaleString("zh-CN");
const fmtDate  = (s) => s;

const CN_NUMS = ["零","一","二","三","四","五","六","七","八","九","十"];
const cnNum = (n) => CN_NUMS[n] || String(n);

const catClass = (c) => {
  switch (c) {
    case "笔":   return "pen";
    case "本":   return "notebook";
    case "尺":   return "ruler";
    case "橡皮": return "eraser";
    default:     return "other";
  }
};

/* Toasts */
const ToastContext = React.createContext(null);
function ToastProvider({ children }) {
  const [items, setItems] = useState([]);
  const push = useCallback((msg, kind = "ok") => {
    const id = Math.random().toString(36).slice(2);
    setItems(prev => [...prev, { id, msg, kind }]);
    setTimeout(() => setItems(prev => prev.filter(t => t.id !== id)), 2600);
  }, []);
  return (
    <ToastContext.Provider value={push}>
      {children}
      <div className="toast-wrap">
        {items.map(t => (
          <div key={t.id} className={"toast " + t.kind}>{t.msg}</div>
        ))}
      </div>
    </ToastContext.Provider>
  );
}
const useToast = () => React.useContext(ToastContext);

/* Modal */
function Modal({ open, onClose, title, sub, children, wide, footer }) {
  useEffect(() => {
    if (!open) return;
    const onKey = e => e.key === "Escape" && onClose();
    window.addEventListener("keydown", onKey);
    return () => window.removeEventListener("keydown", onKey);
  }, [open, onClose]);
  if (!open) return null;
  return ReactDOM.createPortal((
    <div className="modal-backdrop" onClick={onClose}>
      <div className={"modal" + (wide ? " wide" : "")} onClick={e => e.stopPropagation()}>
        <div className="modal-head">
          <div>
            <div className="modal-title">{title}</div>
            {sub && <div className="modal-sub">{sub}</div>}
          </div>
          <button className="modal-close" onClick={onClose} aria-label="关闭">✕</button>
        </div>
        <div className="modal-body">{children}</div>
        {footer && <div className="modal-foot">{footer}</div>}
      </div>
    </div>
  ), document.body);
}

/* Confirm dialog */
function Confirm({ open, title, message, target, confirmLabel = "确认删除", onConfirm, onCancel, busy = false }) {
  return (
    <Modal
      open={open}
      onClose={onCancel}
      title={title || "确认操作"}
      sub="此操作不可撤销"
      footer={
        <>
          <button className="btn" onClick={onCancel} disabled={busy}>取消</button>
          <button className="btn btn-danger" onClick={onConfirm} disabled={busy}>
            {busy ? "处理中…" : confirmLabel}
          </button>
        </>
      }
    >
      <div className="confirm-text">{message}</div>
      {target && <div className="confirm-target">{target}</div>}
    </Modal>
  );
}

function StatsSkeleton({ cells = 4 }) {
  return (
    <div className="stats-strip">
      {Array.from({ length: cells }, (_, idx) => (
        <div className="stat-cell" key={idx}>
          <div className="sk sk-label" />
          <div className="sk sk-value" />
          <div className="sk sk-foot" />
        </div>
      ))}
    </div>
  );
}

function TableSkeletonRows({ rows = 6, cols = 6 }) {
  return (
    <>
      {Array.from({ length: rows }, (_, row) => (
        <tr key={row} className="sk-row">
          {Array.from({ length: cols }, (_, col) => (
            <td key={col}><span className="sk sk-line" /></td>
          ))}
        </tr>
      ))}
    </>
  );
}

/* Ledger pagination */
const TABLE_PAGE_SIZE = 10;

function useTablePagination(items, resetKey, pageSize = TABLE_PAGE_SIZE) {
  const [page, setPage] = useState(1);
  const total = items.length;
  const pageCount = Math.max(1, Math.ceil(total / pageSize));

  useEffect(() => {
    setPage(1);
  }, [resetKey]);

  useEffect(() => {
    setPage(current => Math.min(current, pageCount));
  }, [pageCount]);

  const pageItems = useMemo(() => {
    const start = (page - 1) * pageSize;
    return items.slice(start, start + pageSize);
  }, [items, page, pageSize]);

  return { page, pageCount, pageItems, setPage, total, pageSize };
}

function Pagination({ page, pageCount, total, pageSize = TABLE_PAGE_SIZE, onPageChange }) {
  if (!total) return null;
  const start = (page - 1) * pageSize + 1;
  const end = Math.min(page * pageSize, total);
  const pages = Array.from({ length: pageCount }, (_, index) => index + 1);

  return (
    <div className="pagination" aria-label="账册分页">
      <div className="pagination-summary">第 {start} - {end} 条 · 共 {total} 条</div>
      <div className="pagination-controls">
        <button
          className="pagination-btn"
          disabled={page <= 1}
          onClick={() => onPageChange(page - 1)}
        >上一页</button>
        {pages.map(number => (
          <button
            className={"pagination-page" + (number === page ? " active" : "")}
            aria-current={number === page ? "page" : undefined}
            key={number}
            onClick={() => onPageChange(number)}
          >{String(number).padStart(2, "0")}</button>
        ))}
        <button
          className="pagination-btn"
          disabled={page >= pageCount}
          onClick={() => onPageChange(page + 1)}
        >下一页</button>
      </div>
    </div>
  );
}

/* Category tag */
function CategoryTag({ value }) {
  return <span className={"cat-tag " + catClass(value)}>{value}</span>;
}

/* Sortable header cell */
function Th({ field, sort, setSort, children, align }) {
  const active = sort.field === field;
  const cls = active ? (sort.dir === "asc" ? "sort-asc" : "sort-desc") : "";
  return (
    <th
      className={cls}
      style={align === "right" ? { textAlign: "right" } : null}
      onClick={() => {
        if (active) setSort({ field, dir: sort.dir === "asc" ? "desc" : "asc" });
        else setSort({ field, dir: "asc" });
      }}
    >
      {children}
    </th>
  );
}

/* Persisted state hook */
function usePersistedState(key, initial) {
  const [val, setVal] = useState(() => {
    try {
      const stored = localStorage.getItem(key);
      if (stored) return JSON.parse(stored);
    } catch {}
    return initial;
  });
  useEffect(() => {
    try { localStorage.setItem(key, JSON.stringify(val)); } catch {}
  }, [key, val]);
  return [val, setVal];
}

/* Sorting utility */
function sortBy(arr, field, dir) {
  const m = dir === "asc" ? 1 : -1;
  return [...arr].sort((a, b) => {
    const av = a[field], bv = b[field];
    if (typeof av === "number" && typeof bv === "number") return (av - bv) * m;
    return String(av).localeCompare(String(bv), "zh") * m;
  });
}

Object.assign(window, {
  fmtMoney, fmtInt, fmtDate, cnNum, catClass,
  ToastProvider, useToast, Modal, Confirm,
  StatsSkeleton, TableSkeletonRows,
  Pagination, useTablePagination,
  CategoryTag, Th, usePersistedState, sortBy,
});
