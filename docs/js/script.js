var elmHeader;
var elmDivResult;
var elmInputUrl;
var elmItemTemplate;
var elmStyleList;

function $(id) {
	return document.getElementById(id);
}

function setElements() {
	elmHeader = $("header");
	elmDivResult = $("result");
	elmInputUrl = $("text_url");
	elmItemTemplate = $("item_template");
	elmStyleList = $("style_list");
}

function switchStyle(isForce = false) {
	var disabled = (window.innerWidth >= 480);
	if (isForce || elmStyleList.disabled != disabled) {
		elmStyleList.disabled = disabled;
	}
}

function checkQueryParams() {
	if (window.location.search == "") return;
	elmInputUrl.value = window.location.search.split("?")[1];
	executeProcedure();
}

function executeProcedure() {
	cleanItems();
	setHeaderText("Loading...");
	var url = elmInputUrl.value;
	history.replaceState("", "", "?" + url);
	if (url.startsWith("http://") || url.startsWith("https://")) {
		url = "https://cors-anywhere.herokuapp.com/" + url;
	}
	fetch(url).then(function(response) {
		return response.json();
	}).then(function(json) {
		var text = json.repository;
		if ("maintainer" in json) {
			text += " by " + json.maintainer;
		}
		setHeaderText(escapeHTML(text));
		if ("items" in json) {
			for (var item of json.items) {
				appendItem(item);
			}
			elmDivResult.style.display = "block";
		}
	}).catch(function(err) {
		console.log(err);
		setHeaderText("Error!!");
	});
}

function setHeaderText(text) {
	elmHeader.innerHTML = text;
}

function appendItem(item) {
	var	title = "(no title)",
		info = [],
		imageUrl = "img/blank.gif",
		binaryUrl;
	if ("title" in item) {
		title = item.title;
	} else if ("name" in item) {
		title = item.name;
	}
	if ("version" in item) {
		info.push("version " + item.version);
	}
	if ("author" in item) {
		info.push("by " + item.author);
	}
	if ("genre" in item) {
		info.push("Genre: " + item.genre);
	}
	if ("screenshots" in item && item.screenshots.length >= 1) {
		imageUrl = item.screenshots[0].filename;
	} else if ("cover" in item) {
		imageUrl = item.cover;
	} else if ("banner" in item) {
		imageUrl = item.banner;
	} 
	if ("binaries" in item && item.binaries.length >= 1) {
		binaryUrl = item.binaries[0].filename;
	} else if ("arduboy" in item) {
		binaryUrl = item.arduboy;
	} else if ("hex" in item) {
		binaryUrl = item.hex;
	}

	if (binaryUrl) {
		var elmItem = document.importNode(elmItemTemplate.content, true);
		elmItem.querySelector(".item_image").src = imageUrl;
		elmItem.querySelector(".item_title").innerHTML = title;
		elmItem.querySelector(".item_link_binary").href = binaryUrl;
		elmItem.querySelector(".item_link_arduboy").href = "arduboy:" + binaryUrl;
		var elmItemInfo = elmItem.querySelector(".item_info");
		for (let v of info) {
			var elmTmp = document.createElement('span');
			elmTmp.innerHTML = escapeHTML(v);
			elmItemInfo.appendChild(elmTmp);
		}
		elmDivResult.appendChild(elmItem);
	}
}

function cleanItems() {
	elmDivResult.style.display = "none";
	while (elmDivResult.firstChild) {
		elmDivResult.removeChild(elmDivResult.firstChild);
	}
}

function escapeHTML(html) {
	var elmTmp = document.createElement('div');
	elmTmp.appendChild(document.createTextNode(html));
	return elmTmp.innerHTML;
}
