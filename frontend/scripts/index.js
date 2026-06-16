const API_BASE = "http://localhost:1600";
// TODO: hacer que sea obtenido de input
const DEFAULT_LIMIT = 4;
const BATCHES_PER_PAGE = 3;
//

let currentImgId = null;
let isLoading = false;
let hasMore = true;
let allBlobUrls = [];
let totalImagesLoaded = 0;

const galleryEl = document.getElementById("gallery");
const uploadBtn = document.getElementById("uploadBtn");
const fileInput = document.getElementById("fileInput");
const modal = document.getElementById("imageModal");
const modalImage = document.getElementById("modalImage");
const modalMessage = document.getElementById("modalMessage");
const downloadOriginalBtn = document.getElementById("downloadOriginalBtn");
const closeModalBtn = document.getElementById("closeModalBtn");
const loaderContainer = document.getElementById("loaderContainer");
const loadMoreBtn = document.getElementById("loadMoreBtn");
const loadMoreBtnLoadMoreText = `Load (${BATCHES_PER_PAGE * DEFAULT_LIMIT} images)`;
const loadMoreBtnNoMoreText = `No more images`;
loadMoreBtn.textContent = loadMoreBtnLoadMoreText;

let currentModalId = null;

function showError(msg) {
  const toast = document.createElement("div");
  toast.className = "error-toast";
  toast.innerText = msg;
  document.body.appendChild(toast);
  setTimeout(() => toast.remove(), 4000);
}

function setLoading(loading) {
  isLoading = loading;
  if (loading) {
    loaderContainer.innerHTML = '<div class="spinner"></div><p>Loading...</p>';
    loadMoreBtn.disabled = true;
  } else {
    loaderContainer.innerHTML = "";
    if (!hasMore) {
      loaderContainer.innerHTML = '<p style="color:#a0a0a0;">✨ END :) ✨</p>';
      loadMoreBtn.disabled = true;
      loadMoreBtn.textContent = loadMoreBtnNoMoreText;
    } else {
      loadMoreBtn.disabled = false;
      loadMoreBtn.textContent = loadMoreBtnNoMoreText;
    }
  }
}

function revokeAllBlobs() {
  // basically a free
  for (let url of allBlobUrls) URL.revokeObjectURL(url);
  allBlobUrls = [];
}

function addImageToGallery(imgId, imageBlob) {
  console.log(imageBlob);
  const blobUrl = URL.createObjectURL(imageBlob);
  allBlobUrls.push(blobUrl);

  const card = document.createElement("div");
  card.className = "card";

  const img = document.createElement("img");
  img.className = "card-img";
  img.src = blobUrl;
  img.loading = "lazy";
  img.alt = `Imagen ${imgId}`;

  card.appendChild(img);
  card.addEventListener("click", () => openImageModal(imgId));

  galleryEl.insertBefore(card, loaderContainer);

  totalImagesLoaded++;
}

async function parseCursorResponse(arrayBuffer) {
  const dv = new DataView(arrayBuffer);
  let offset = 0;
  const images = [];

  while (offset + 18 <= arrayBuffer.byteLength) {
    const sequentialIdx = dv.getUint16(offset, true);
    const img_id = dv.getBigUint64(offset + 2, true);
    const img_size = dv.getBigUint64(offset + 10, true);

    offset += 18;
    if (offset + Number(img_size) > arrayBuffer.byteLength) {
      break;
    }
    const imageData = arrayBuffer.slice(offset, offset + Number(img_size));
    const imageBlob = new Blob([imageData], { type: "image/webp" });
    images.push({
      sequential_idx: sequentialIdx,
      img_id: Number(img_id),
      img_size: Number(img_size),
      blob: imageBlob,
    });
    offset += Number(img_size);
  }
  console.log(images);
  return images;
}

async function fetchSingleBatch() {
  if (!hasMore || currentImgId === null || currentImgId < 0) {
    console.log("Early return on fetchSingleBatch");
    return [];
  }

  let dynamicLimit = DEFAULT_LIMIT;
  if (currentImgId <= DEFAULT_LIMIT) {
    dynamicLimit = currentImgId - 1;
  }

  if (dynamicLimit <= 1) {
    hasMore = false;
    return [];
  }

  const url = `${API_BASE}/api/image/cursor?current=${currentImgId}&limit=${dynamicLimit}`;

  try {
    const response = await fetch(url);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    const arrayBuffer = await response.arrayBuffer();
    if (arrayBuffer.byteLength === 0) {
      console.log("byteLength === 0");
      return [];
    }
    return await parseCursorResponse(arrayBuffer);
  } catch (err) {
    console.error("Error fetching batch:", err);
    throw err;
  }
}

async function loadMultipleBatches() {
  if (isLoading) return;
  if (!hasMore) return;
  if (currentImgId === null || currentImgId < 0) {
    hasMore = false;
    setLoading(false);
    return;
  }

  isLoading = true;
  setLoading(true);
  let batchesLoaded = 0;
  let imagesAddedInThisLoad = 0;

  try {
    while (
      batchesLoaded < BATCHES_PER_PAGE &&
      hasMore &&
      currentImgId !== null &&
      currentImgId >= 0
    ) {
      const images = await fetchSingleBatch();
      console.log(images);

      if (images.length === 0) {
        hasMore = false;
        break;
      }

      for (const img of images) {
        addImageToGallery(img.img_id, img.blob);
        imagesAddedInThisLoad++;
      }
      batchesLoaded++;

      const imgIds = images.map((i) => i.img_id);
      const minImgId = Math.min(...imgIds);
      const nextCurrent = minImgId - 1;

      if (nextCurrent < 0) {
        hasMore = false;
        currentImgId = null;
      } else if (
        images.length < DEFAULT_LIMIT &&
        currentImgId + 1 <= DEFAULT_LIMIT
      ) {
        hasMore = false;
        currentImgId = null;
      } else {
        currentImgId = nextCurrent + 1;
      }

      await new Promise((r) => setTimeout(r, 50));
    }
  } catch (err) {
    console.log(err.message);
    showError(`Error loading image batches: ${err.message}`);
  } finally {
    isLoading = false;
    setLoading(false);
  }
}

async function openImageModal(imgId) {
  currentModalId = imgId;
  modal.style.display = "flex";
  modalImage.src = "";
  modalMessage.innerText = "Loading compressed version";

  try {
    const compressedUrl = `${API_BASE}/api/image?variant=compressed&id=${imgId}`;
    const resp = await fetch(compressedUrl);
    if (!resp.ok) throw new Error(`Error compressed: ${resp.status}`);
    const blob = await resp.blob();
    const url = URL.createObjectURL(blob);
    modalImage.src = url;
    modalMessage.innerText =
      "📸 To see the image on original resolution please press the button";
    modalImage.onload = () => URL.revokeObjectURL(url);
  } catch (err) {
    modalMessage.innerText = `Error: ${err.message}`;
  }
}

async function downloadOriginalImage() {
  if (!currentModalId) return;
  try {
    const url = `${API_BASE}/api/image?variant=original&id=${currentModalId}`;
    const resp = await fetch(url);
    if (!resp.ok) throw new Error(`Error original: ${resp.status}`);
    const blob = await resp.blob();
    const link = document.createElement("a");
    const objUrl = URL.createObjectURL(blob);
    link.href = objUrl;
    const contentType = resp.headers.get("content-type") || "image/jpeg";
    const extension = contentType.split("/")[1] || "jpg";
    link.download = `image_${currentModalId}_original.${extension}`;
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
    URL.revokeObjectURL(objUrl);
  } catch (err) {
    showError(`Couldnt download original: ${err.message}`);
  }
}

async function uploadImage(file) {
  if (!file) return;

  const reader = new FileReader();
  reader.onload = async (e) => {
    const arrayBuffer = e.target.result;
    try {
      const response = await fetch(`${API_BASE}/api/image`, {
        method: "POST",
        headers: { "Content-Length": arrayBuffer.byteLength.toString() },
        body: arrayBuffer,
      });

      if (!response.ok)
        throw new Error(`Failed to upload image: ${response.status}`);
      showError("✅ Reloading gallery...");
      await initGallery();
    } catch (err) {
      showError(`Error uploading: ${err.message}`);
    }
  };
  reader.readAsArrayBuffer(file);
}

async function initGallery() {
  revokeAllBlobs();
  const cards = document.querySelectorAll(".card");
  cards.forEach((card) => card.remove());

  hasMore = true;
  isLoading = false;
  currentImgId = null;
  totalImagesLoaded = 0;

  loadMoreBtn.disabled = false;
  loadMoreBtn.textContent = loadMoreBtnLoadMoreText;

  try {
    const startResp = await fetch(`${API_BASE}/api/image/cursor/start`);
    if (!startResp.ok) throw new Error(`start error: ${startResp.status}`);
    const text = await startResp.text();
    const maxIdx = parseInt(text.trim(), 10);
    if (isNaN(maxIdx)) throw new Error("Invalid start number");
    currentImgId = maxIdx;
    await loadMultipleBatches();
  } catch (err) {
    showError(`Failed: ${err.message}`);
    if (loaderContainer) {
      loaderContainer.innerHTML =
        '<p style="color: #ff6b6b;">❌ Connection error.</p>';
    }
    if (loadMoreBtn) loadMoreBtn.disabled = true;
  }
}
function closeModal() {
  modal.style.display = "none";
  if (modalImage.src && modalImage.src.startsWith("blob:"))
    URL.revokeObjectURL(modalImage.src);
  currentModalId = null;
}

// Ev L
uploadBtn.addEventListener("click", () => fileInput.click());
fileInput.addEventListener("change", (e) => {
  if (e.target.files?.[0]) uploadImage(e.target.files[0]);
  fileInput.value = "";
});

window.addEventListener("click", (e) => {
  if (e.target === modal) closeModalBtn.click();
});

initGallery();
