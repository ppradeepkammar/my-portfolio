/* =========================
   IMAGE POPUP
   ========================= */

function openImage(imageElement) {

    const modal = document.getElementById("imageModal");
    const expandedImage = document.getElementById("expandedImage");

    if (!modal || !expandedImage) {
        return;
    }

    expandedImage.src = imageElement.currentSrc || imageElement.src;
    expandedImage.alt = imageElement.alt;

    modal.style.display = "flex";
    modal.setAttribute("aria-hidden", "false");
}


/* Close image */

function closeImage() {

    const modal = document.getElementById("imageModal");

    if (!modal) {
        return;
    }

    modal.style.display = "none";
    modal.setAttribute("aria-hidden", "true");
}


/* Close when clicking outside image */

const imageModal = document.getElementById("imageModal");

if (imageModal) {
    imageModal.addEventListener("click", function(event) {

        if (event.target === this) {
            closeImage();
        }

    });
}


/* Close with ESC key */

document.addEventListener("keydown", function(event) {

    if (event.key === "Escape") {
        closeImage();
    }

});


/* Open every project image through one delegated handler. */

const projectDetailsPage = document.querySelector(".project-details-page");

if (projectDetailsPage) {

    projectDetailsPage.querySelectorAll("img:not(.expanded-image)").forEach(function(image) {
        image.tabIndex = 0;
        image.setAttribute("role", "button");
        image.setAttribute("aria-label", "Open larger image: " + image.alt);
    });

    projectDetailsPage.addEventListener("click", function(event) {
        const image = event.target.closest("img:not(.expanded-image)");

        if (image && projectDetailsPage.contains(image)) {
            openImage(image);
        }
    });

    projectDetailsPage.addEventListener("keydown", function(event) {
        if (event.target.matches("img:not(.expanded-image)") && (event.key === "Enter" || event.key === " ")) {
            event.preventDefault();
            openImage(event.target);
        }
    });

}