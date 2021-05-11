function loadData(url) {
	var json = null;
	$.ajax({
		'async': false,
		'global': false,
		'url': url,
		'dataType': 'json',
		'success': function(data) {
			json = data;
		}
	});
	return json;
}

// Load JSON data
var df = loadData('data.json');
var data = df.data;
var C = df.columns;
var path_loss_models = df.path_loss_models;

var gateways = loadData('gateways.json');

for (var gw in gateways) {
	$('<input type="radio" name="gw" id="' + gw + '" onclick="drawData();"/>' + gateways[gw].name + '<br/>')
		.appendTo('#gw_select');
}
$('input[name=gw]').first().prop('checked', true);

$('#enable_screenshot_mode').checkboxradio();
$('#enable_screenshot_mode').on('change', function() {
	if (this.checked) {
		map.dragging.disable();
		map.scrollWheelZoom.disable();
	} else {
		map.dragging.enable();
		map.scrollWheelZoom.enable();
	}
	$('.info.legend.leaflet-control').draggable({disabled: !this.checked});
	$('.leaflet-control-layers.leaflet-control').toggle();
	$('.leaflet-control-zoom.leaflet-bar.leaflet-control').toggle();
});


function get_sf(d) {
	rpp = get_rpp(d);
	if (rpp >= -126.5) return 7;
	if (rpp >= -129)   return 8;
	if (rpp >= -131.5) return 9;
	if (rpp >= -134)   return 10;
	if (rpp >= -136)   return 11;
	if (rpp >= -139.5) return 12;
	return 12;
}

function get_rpp(d) {
	return d[C.rssi] + Math.min(d[C.snr], 0);
}

function get_model_diff(d, model) {
	return Math.abs(get_rpp(d) - (14 - 2 + 5 - d[C[model]]));
}

var rpp_min = Number.POSITIVE_INFINITY;
var rpp_max = Number.NEGATIVE_INFINITY;
var model_diff_min = Number.POSITIVE_INFINITY;
var model_diff_max = Number.NEGATIVE_INFINITY;
for (var gw in data) {
	data[gw].forEach(function(d) {
		rpp = get_rpp(d);
		rpp_min = Math.min(rpp_min, rpp);
		rpp_max = Math.max(rpp_max, rpp);
		for (var model in path_loss_models) {
			diff = get_model_diff(d, model);
			model_diff_min = Math.min(model_diff_min, diff);
			model_diff_max = Math.max(model_diff_max, diff);
		} 
	});
}

function roundToTwo(num) {
	return +(Math.round(num + 'e+2')  + 'e-2');
}

function range(start, end) {
	return new Array(end - start).fill().map((d, i) => i + start);
}


var osmLayer = L.tileLayer('http://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
	attribution: '&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors',
	maxZoom: 19,
});

googleLayer = L.tileLayer('http://{s}.google.com/vt/lyrs=s&x={x}&y={y}&z={z}',{
	maxZoom: 20,
	attribution: 'Tiles &copy; Google',
	subdomains:['mt0','mt1','mt2','mt3']
});

mapLink = '<a href="http://www.esri.com/">Esri</a>';
wholink = 'i-cubed, USDA, USGS, AEX, GeoEye, Getmapping, Aerogrid, IGN, IGP, UPR-EGP, and the GIS User Community';
esriLayer = L.tileLayer(
	'http://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}', {
	attribution: 'Tiles &copy; '+mapLink+', '+wholink,
	maxZoom: 18,
});

var map = L.map('map', {
	layers: [osmLayer],
	center: L.latLng(50.70743, 7.0982068), zoom: 11
});

L.control.layers({
	"Streets": osmLayer, "Satellite (Google)": googleLayer, "Satellite (Esri)": esriLayer}, {}).addTo(map);

var hexLayer;
var legend;
var gwMarkers = [];
var gwIcon = L.divIcon({
	className: 'gw-icon',
	html: "<div class='marker-pin'></div><i class='fa fa-broadcast-tower awesome'>",
	iconSize: [30, 42],
	iconAnchor: [15, 42]
});


function drawContinuousLegend()
{
	// https://gist.github.com/syntagmatic/e8ccca52559796be775553b467593a9f

	var legendheight = 200,
		legendwidth = 80,
		margin = {top: 20, right: 60, bottom: 10, left: 2};

	var canvas = d3.select('#legend')
		.style('height', legendheight + 'px')
		.style('width', legendwidth-20 + 'px')
		.style('position', 'relative')
		.append('canvas')
		.attr('height', legendheight - margin.top - margin.bottom)
		.attr('width', 1)
		.style('height', (legendheight - margin.top - margin.bottom) + 'px')
		.style('width', (legendwidth - margin.left - margin.right) + 'px')
		.style('border', '1px solid #000')
		.style('position', 'absolute')
		.style('top', (margin.top) + 'px')
		.style('left', (margin.left) + 'px')
		.node();

	var ctx = canvas.getContext('2d');

	var legendscale = d3.scaleLinear()
		.range([1, legendheight - margin.top - margin.bottom])
		.domain(this.legendScaleDomain);
	
	var legendFillColor = this.legendFillColor;
	var colorScale = this.colorScale;
	d3.range(legendscale.range()[1]+1).forEach(function(i) {
		ctx.fillStyle = legendFillColor(legendscale, colorScale, i);
		ctx.fillRect(0,i,1,1);
	});

	var legendaxis = d3.axisRight()
		.scale(legendscale)
		.tickSize(6)
		.ticks(8);

	d3.select('#legend')
		.append('svg')
		.attr('height', (legendheight) + 'px')
		.attr('width', (legendwidth) + 'px')
		.style('position', 'absolute')
		.style('left', '0px')
		.style('top', '0px')
		.append('g')
		.attr('class', 'axis')
		.attr('transform', 'translate(' + (legendwidth - margin.left - margin.right + 3) + ',' + (margin.top) + ')')
		.call(legendaxis);
}


function drawValueLegend()
{
	var colorScale = this.colorScale;
	range(this.legendScaleDomain[0], this.legendScaleDomain[1]+1).forEach(function(s) {
		$('#legend')[0].innerHTML +=
			'<br><i style="background:' + colorScale(s) + '"></i> ' + s;
	});
}


var colorizationDescriptor = {
	rpp: {
		title: 'Received Packet Power (RPP)',
		colorDomain: [0, Math.abs(rpp_min)+rpp_max],
		colorValue: function(d) { return d.rpp_avg + Math.abs(rpp_min); },
		legendTitle: 'RPP',
		drawLegend: drawContinuousLegend,
		legendFillColor: function(legendscale, colorScale, i) { return colorScale(legendscale.invert(i) + Math.abs(rpp_min)); },
		legendScaleDomain: [rpp_min, rpp_max],
	},
	sf: {
		title: 'Spreading Factor (SF)',
		colorDomain: [12, 7],
		colorValue: function(d) { return d.sf_avg; },
		legendTitle: 'SF',
		drawLegend: drawValueLegend,
		legendFillColor: function(legendscale, colorScale, i) { return colorScale(legendscale.invert(i)); },
		legendScaleDomain: [7, 12],
	},
	model_diff: {
		title: 'Path Loss Model Deviation',
		colorDomain: [50, 0],
		colorValue: function(d) { return d.model_diff_avg; },
		legendTitle: 'Deviation',
		drawLegend: drawContinuousLegend,
		legendFillColor: function(legendscale, colorScale, i) { return colorScale(legendscale.invert(i)); },
		legendScaleDomain: [model_diff_min, 50],
	},
};

for (var c in colorizationDescriptor) {
	$('<input type="radio" name="colorization" id="' + c + '" onclick="drawData()"/><span id="' + c +'_append">' +
			colorizationDescriptor[c].title + '</span><br/>')
		.appendTo('#color_select');
}
$('input[name=colorization]').first().prop('checked', true);

$('</br><select id="model_diff_select"></select>').appendTo('#model_diff_append');
for (var m in path_loss_models) {
	$('<option value="' + m + '">' + path_loss_models[m][0] + '</option>').appendTo('#model_diff_select');
}
$('#model_diff_select').selectmenu({change: drawData});


$(document).tooltip({
	content: function(callback) {
		callback($(this).prop('title')); },
	position: {
		my: "center bottom",
		at: "right top"
	}
});
$("#data_filter").autocomplete({
	source: ['rpp', 'rssi', 'snr', 'sf', 'sensor', 'distance']
});
$("#apply_filter").click(function() { drawData(); });
$("#clear_filter").click(function() { 
	$("#data_filter").val('');
	$("#filter_error").hide('fade'); 
	drawData(); });
$("#data_filter").on('keyup', function(e) {
	if (e.key === 'Enter' || e.keyCode === 13) drawData(); });


function getTooltip(d) {
	if (d.length > 1)
		return `Aggregated values: ${d.length}<br>
				RPP: ${d.rpp_avg}<br>
				SF: ${d.sf_avg}`;
	o = d[0].o;
	lat = o[C.latitude_snapped].toString();
	lon = o[C.longitude_snapped].toString();
	gw = $('input[name=gw]:checked')[0].id;
	tt= `<div style="float:left; padding-right:5px">RPP: ${get_rpp(o)}<br>
			RSSI: ${o[C.rssi]}<br> 
			SNR: ${o[C.snr]}<br>
			SF: ${get_sf(o)}<br>
			Distance: ${o[C.gw_distance_rounded_km]} km<br>
			<br>Model Deviations:`;
	for (var m in path_loss_models) {
		tt += `<br>${path_loss_models[m][1]}: ${roundToTwo(get_model_diff(o, m))}`;
	}
	/*
	tt += `</div><div style="float:left; border-left:1px solid;">
			<img src="plots/${lat.slice(0,6)}/${lat.slice(6)}/${lat},${lon}-to-${gw}.splat.png" width="384px">
			<img src="plots/${lat.slice(0,6)}/${lat.slice(6)}/${lat},${lon}-to-${gw}.signalserver.lidar.png" width="384px"></div>`;
	*/
	return tt;
}


function drawData() {
	var selected_gw = $('input[name=gw]:checked')[0].id;
	var selected_colorization = $('input[name=colorization]:checked')[0].id;

	var colorization = colorizationDescriptor[selected_colorization];

	if (hexLayer)
		$(".hexbin").parent( "g" ).remove();  // https://github.com/Asymmetrik/leaflet-d3/issues/68
		//map.removeLayer(hexLayer)

	hexLayer = L.hexbinLayer({ duration: 100, colorDomain: colorization.colorDomain, opacity: 0.5 })
		.colorValue(function(d, i) {
			d.rpp_avg = roundToTwo(d.reduce(function(acc, obj) { return acc + get_rpp(obj.o) / d.length; }, 0));
			d.sf_avg = roundToTwo(d.reduce(function(acc, obj) { return acc + get_sf(obj.o) / d.length; }, 0));
			if (selected_colorization == 'model_diff') {
				var selected_model_diff = $('#model_diff_select option:checked')[0].value;
				d.model_diff_avg = roundToTwo(d.reduce(function(acc, obj) {
					return acc + get_model_diff(obj.o, selected_model_diff) / d.length; }, 0));
			}
			return colorization.colorValue(d);
		})
		.hoverHandler(L.HexbinHoverHandler.compound({
			handlers: [
				L.HexbinHoverHandler.resizeFill(),
				L.HexbinHoverHandler.tooltip({ tooltipContent: getTooltip })
			]
		}));

	colorization.colorScale = d3.scaleSequential(d3.interpolateRdYlGn)
		.domain(hexLayer.options.colorDomain);

	hexLayer.colorScale(colorization.colorScale);

	hexLayer.addTo(map);

	if (legend)
		map.removeControl(legend);

	legend = L.control({position: 'bottomright'});
	legend.onAdd = function(map) {
		var div = L.DomUtil.create('div', 'info legend');
		div.innerHTML +=
			'<div id="legend" style="display: inline-block">' + colorization.legendTitle + '</div>';
		return div;
	};
	legend.addTo(map);

	colorization.drawLegend();

	var filter = $('#data_filter').val();
	if (filter) {
		try {
			f = Function('"use strict"; return (' + filter + ')');
			hexLayer.data(data[selected_gw].filter(function(d) { 
				rpp = get_rpp(d); snr = d[C.snr]; sf = get_sf(d); rssi = d[C.rssi];
				sensor = d[C.dev_id]; distance = d[C.gw_distance_rounded_km];
				return f(); }));
			$('#filter_error').hide('fade');
		} catch (err) {
			$('#filter_error_msg').text(err.toString());
			$('#filter_error').show('fade');
		}
	 } else {
		hexLayer.data(data[selected_gw]);
	 }

	gwMarkers.forEach(m => map.removeLayer(m));
	gwMarkers.length = 0;
	for (var gw in gateways) {
		if ((selected_gw != 'all' && selected_gw == gw) || (selected_gw == 'all' && selected_gw != gw)) {
			marker = new L.marker([gateways[gw].latitude, gateways[gw].longitude], { icon: gwIcon })
				.bindTooltip(gateways[gw].name);
			gwMarkers.push(marker);
		}
	}
	gwMarkers.forEach(m => m.addTo(map));
}

$('input[name=gw]').first().click();
