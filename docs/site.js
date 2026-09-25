// Links to the GitHub repository, worked out from the Pages address
// (owner.github.io/repo/ -> github.com/owner/repo), and the screenshot viewer.
(function () {
	var host = location.hostname, repo = null;
	var m = host.match(/^([^.]+)\.github\.io$/i);
	if (m) {
		var first = location.pathname.split("/").filter(Boolean)[0];
		repo = "https://github.com/" + m[1] + "/" + (first && !/\.html?$/.test(first) ? first : host);
	}
	document.querySelectorAll("[data-gh]").forEach(function (a) {
		var path = a.getAttribute("data-gh");
		if (repo)
			a.href = repo + (path ? "/" + path : "");
		else if (!a.getAttribute("href"))
			a.href = "https://github.com/search?q=typing-adventure+pi-typing&type=repositories";
	});

	// Screenshot viewer: click a screenshot to see it full size, arrows step through them.
	var dialog = document.getElementById("lightbox");
	if (!dialog || !dialog.showModal)
		return;
	var buttons = Array.prototype.slice.call(document.querySelectorAll("figure.shot button"));
	var img = dialog.querySelector("img"), cap = dialog.querySelector("p"), at = 0;
	function show(i) {
		at = (i + buttons.length) % buttons.length;
		var fig = buttons[at].closest("figure"), small = buttons[at].querySelector("img");
		img.src = small.src;
		img.alt = small.alt;
		var title = fig.querySelector("figcaption strong");
		cap.textContent = title ? title.textContent : small.alt;
	}
	buttons.forEach(function (b, i) {
		b.addEventListener("click", function () {
			show(i);
			dialog.showModal();
		});
	});
	dialog.querySelector(".lb-prev").addEventListener("click", function () { show(at - 1); });
	dialog.querySelector(".lb-next").addEventListener("click", function () { show(at + 1); });
	dialog.querySelector(".lb-close").addEventListener("click", function () { dialog.close(); });
	dialog.addEventListener("click", function (e) {
		if (e.target === dialog)
			dialog.close();
	});
	dialog.addEventListener("keydown", function (e) {
		if (e.key === "ArrowLeft") show(at - 1);
		if (e.key === "ArrowRight") show(at + 1);
	});
})();
