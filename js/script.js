/* =========================
   IMAGE POPUP
   ========================= */

function openImage(imageElement) {

    const modal = document.getElementById("imageModal");
    const expandedImage = document.getElementById("expandedImage");

    expandedImage.src = imageElement.src;

    modal.style.display = "flex";
}


/* Close image */

function closeImage() {

    const modal = document.getElementById("imageModal");

    modal.style.display = "none";
}


/* Close when clicking outside image */

document.getElementById("imageModal").addEventListener("click", function(event) {

    if (event.target === this) {
        closeImage();
    }

});


/* Close with ESC key */

document.addEventListener("keydown", function(event) {

    if (event.key === "Escape") {
        closeImage();
    }

});