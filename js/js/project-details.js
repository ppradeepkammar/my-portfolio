const projectNumber = new URLSearchParams(window.location.search).get("project");

if (projectNumber && projectNumber !== "1") {
    const titleElement = document.getElementById("project-title");
    const unavailableSection = document.getElementById("project-unavailable");
    const overviewElement = document.getElementById("project-overview");

    if (titleElement && unavailableSection && overviewElement) {
        const project = projectNumber === "2"
            ? { title: "Driver", description: "Driver" }
            : {
                title: "Project details unavailable",
                description: "Details for this project are not available."
            };

        titleElement.textContent = project.title;
        overviewElement.textContent = project.description;
        unavailableSection.hidden = false;

        Array.from(document.body.children).forEach(function(element) {
            if (!element.matches(
                ".project-detail-header, .project-unavailable, .floating-back-button, .image-modal, script"
            )) {
                element.hidden = true;
            }
        });
    }
}