L.AnimatedMarker = L.Marker.extend({
  options: {
    // meters
    distance: 200,
    // ms
    interval: 1000,
    // animate on add?
    autoStart: true,
    // callback onend
    onEnd: function(){},
    // marker clickable
    clickable: true,
    
    //location descriptions
    geo_descriptions: [],
    geo_imgs: []
  },

  initialize: function (latlngs, options) {
    this.setLine(latlngs);
    L.Marker.prototype.initialize.call(this, latlngs[0], options);
  },

  // Breaks the line up into tiny chunks (see options) ONLY if CSS3 animations
  // are not supported.
  _chunk: function(latlngs) {
    var i,
        len = latlngs.length,
        chunkedLatLngs = [];

    for (i=1;i<len;i++) {
      var cur = latlngs[i-1],
          next = latlngs[i],
          dist = cur.distanceTo(next),
          factor = this.options.distance / dist,
          dLat = factor * (next.lat - cur.lat),
          dLng = factor * (next.lng - cur.lng);

      if (dist > this.options.distance) {
        while (dist > this.options.distance) {
          cur = new L.LatLng(cur.lat + dLat, cur.lng + dLng);
          dist = cur.distanceTo(next);
          chunkedLatLngs.push(cur);
        }
      } else {
        chunkedLatLngs.push(cur);
      }
    }
    chunkedLatLngs.push(latlngs[len-1]);

    return chunkedLatLngs;
  },

  onAdd: function (map) {
    L.Marker.prototype.onAdd.call(this, map);

    // Start animating when added to the map
    if (this.options.autoStart) {
      this.start();
    }
  },

  animate: function() {
    var self = this,
        len = this._latlngs.length,
        speed = this.options.interval;

    // Normalize the transition speed from vertex to vertex
    if (this._i < len) {
      speed = this._latlngs[this._i-1].distanceTo(this._latlngs[this._i]) / this.options.distance * this.options.interval;
    }

    // Only if CSS3 transitions are supported
    if (L.DomUtil.TRANSITION) {
      if (this._icon) { this._icon.style[L.DomUtil.TRANSITION] = ('all ' + speed + 'ms linear'); }
      if (this._shadow) { this._shadow.style[L.DomUtil.TRANSITION] = 'all ' + speed + 'ms linear'; }
    }

    // Move to the next vertex
    // two ways of showing popup message
    /*
     * 1) this.bindPopup('Hello').openPopup();
     * 2) var popup = L.popup()
	     .setLatLng(this._latlngs[this._i-1])
	     .setContent('<p style="color:#000;">Hello world!<br />This is a nice popup.</p>')
	     .openOn(map);*/
    
    //draw polyline
    polyline = L.polyline([this._latlngs[this._i - 1], this._latlngs[this._i]], {color: '#ee8c00', weight: 2, "dashArray" : "5, 1, 5"}).addTo(map);
    
    //add popup layers
    if(this._i < len - 1){
        var popupLocation1 = this._latlngs[this._i];
        var popupContent1 = '<img alt="taipei_station" src="'+ this.options.geo_imgs[this._i] +'" style="width: 150px;"><p style="color:#000;">'+ this.options.geo_descriptions[this._i] +'</p>',
        popup1 = new L.Popup();

        popup1.setLatLng(popupLocation1);
        popup1.setContent(popupContent1);
        map.addLayer(popup1);
    }
    
    // circle setting
    circle = L.circle(this._latlngs[this._i], 15, {
	    color: '#f00',
	    fillColor: '#f06',
	    fillOpacity: 0.5
	}).bindPopup('<img alt="taipei_station" src="'+ this.options.geo_imgs[this._i] +'" style="width: 200px;"><p style="color:#000;">'+ this.options.geo_descriptions[this._i] +' </p>', {keepInView: true}).addTo(map).openPopup();
    
    //set center of map view
    map.setView(this._latlngs[this._i],2); //openOn( <Map> map )
    
    //set position of marker
    this.setLatLng(this._latlngs[this._i]);
    this._i++;

    // Queue up the animation to the next next vertex
    this._tid = setTimeout(function(){
      if (self._i === len) {
        self.options.onEnd.apply(self, Array.prototype.slice.call(arguments));
      } else {
        self.animate();
      }
    }, speed);
  },

  // start the animation
  start: function() {
    this.animate();
  },

  // stop the animation in place
  stop: function() {
    if (this._tid) {
      clearTimeout(this._tid);
    }
  },

  setLine: function(latlngs){
    if (L.DomUtil.TRANSITION) {
      // No need to to check up the line if we can animate using CSS3
      this._latlngs = latlngs;
    } else {
      // Chunk up the lines into options.distance bits
      this._latlngs = this._chunk(latlngs);
      this.options.distance = 10;
      this.options.interval = 30;
    }
    this._i = 1;
  }

});

L.animatedMarker = function (latlngs, options) {
  return new L.AnimatedMarker(latlngs, options);
};