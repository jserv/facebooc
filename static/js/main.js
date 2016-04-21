/**/
(function($) {
  // Closes the Responsive Menu on Menu Item Click
  $('.navbar-collapse ul li a').click(function() {
      $('.navbar-toggle:visible').click();
  });

  angular.element(document).ready(function(){
    angular.bootstrap(document.body, ['mainApp']);
  });

  window.main_app = window.main_app || angular.module('mainApp', [ 'ngAnimate' ], function($interpolateProvider) {
    $interpolateProvider.startSymbol('[[');
    $interpolateProvider.endSymbol(']]');
  });

  window.main_app.value('GLOBAL_VALUES',{
    EMAIL : 'service@facebooc.com',
    TITLE : 'Facebooc',
    BROWSER_CHECKER: {
      // Opera 8.0+
      isOpera: ( (!!window.opr && !!opr.addons) || !!window.opera || navigator.userAgent.indexOf(' OPR/') >= 0 ),
      // Firefox 1.0+
      isFirefox: ( typeof InstallTrigger !== 'undefined' ),
      // At least Safari 3+: "[object HTMLElementConstructor]"
      isSafari: ( Object.prototype.toString.call(window.HTMLElement).indexOf('Constructor') > 0 ),
      // Internet Explorer 6-11
      isIE: /*@cc_on!@*/false || !!document.documentMode,
      // Edge 20+
      isEdge: ( !(false || !!document.documentMode) && !!window.StyleMedia ),
      // Chrome 1+
      isChrome: ( !!window.chrome && !!window.chrome.webstore ),
      // Blink engine detection
      isBlink: ( ( ( !!window.chrome && !!window.chrome.webstore ) || ( (!!window.opr && !!opr.addons) || !!window.opera || navigator.userAgent.indexOf(' OPR/') >= 0 ) ) && !!window.CSS )
    }
  });

  /* controllers */
  // headerController
  var headerController = function($rootScope, $scope, $http, $window, $location, GLOBAL_VALUES){
    var ctrl = this;
    ctrl.init_header = function(){
      console.log('Hello Facebooc');
      T("audio").load("/static/timbre/audio/drumkit.wav", function() {
        var BD  = this.slice(   0,  500).set({bang:false});
        var SD  = this.slice( 500, 1000).set({bang:false});
        var HH1 = this.slice(1000, 1500).set({bang:false, mul:0.2});
        var HH2 = this.slice(1500, 2000).set({bang:false, mul:0.2});
        var CYM = this.slice(2000).set({bang:false, mul:0.2});
        var scale = new sc.Scale([0,1,3,7,8], 12, "Pelog");

        var P1 = [
          [BD, HH1],
          [HH1],
          [HH2],
          [],
          [BD, SD, HH1],
          [HH1],
          [HH2],
          [SD],
        ].wrapExtend(128);

        var P2 = sc.series(16);

        var drum = T("lowshelf", {freq:110, gain:8, mul:0.6}, BD, SD, HH1, HH2, CYM).play();
        var lead = T("saw", {freq:T("param")});
        var vcf  = T("MoogFF", {freq:2400, gain:6, mul:0.1}, lead);
        var env  = T("perc", {r:100});
        var arp  = T("OscGen", {wave:"sin(15)", env:env, mul:0.5});

        T("delay", {time:"BPM128 L4", fb:0.65, mix:0.35}, 
          T("pan", {pos:0.2}, vcf), 
          T("pan", {pos:T("tri", {freq:"BPM64 L1", mul:0.8}).kr()}, arp)
        ).play();

        T("interval", {interval:"BPM128 L16"}, function(count) {
          var i = count % P1.length;
          if (i === 0) CYM.bang();

          P1[i].forEach(function(p) { p.bang(); });

          if (Math.random() < 0.015) {
            var j = (Math.random() * P1.length)|0;
            P1.wrapSwap(i, j);
            P2.wrapSwap(i, j);
          }

          var noteNum = scale.wrapAt(P2.wrapAt(count)) + 60;
          if (i % 2 === 0) {
            lead.freq.linTo(noteNum.midicps() * 2, "100ms");
          }
          arp.noteOn(noteNum + 24, 60);
        }).start();
      });
    }
  }
  headerController.$injector = ['$rootScope', '$scope', '$http', '$window', '$location', 'GLOBAL_VALUES'];
  window.main_app.controller('headerCtrl', headerController);

  // headerController
  var frontPageController = function($rootScope, $scope, $http, $window, $location, GLOBAL_VALUES){
    var ctrl = this;
    ctrl.map_handler = ctrl.map_handler || {
      map : null,
      tiles : null,
      my_icon : null,
      d3_svg : null,
      d3_gradient : null,
      d3_data : null,
      d3_circles : null,
      is_set_map_center_on : true,
      default_location : [23.893589, 121.083589],
      default_circles : [[23.893589, 121.083589]],
      location_ary : [],
      tweets_list : [],
      is_appending : false,
      init_map : function(arg_position){
        var _this = this;
        // set geo-location
        if( arg_position &&
            arg_position.coords){
      	  _this.default_location = [arg_position.coords.latitude, arg_position.coords.longitude];
      	  _this.default_circles = [[arg_position.coords.latitude, arg_position.coords.longitude]];
      	}
        // init map
        if(!_this.map){
          // init map
          _this.map = new L.map('front_page')
                          .setView( _this.default_location, 8);

          // set map tiles
          L.tileLayer(
            'http://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
            attribution: '© + Openstreetmap Contributors',
            maxZoom: 18,
          }).addTo(_this.map);

          // init path root of map
          _this.map._initPathRoot();

          // set d3
          if(!_this.d3_svg){
            _this.d3_svg = d3.select("div#front_page").select("svg");
          }

          // define the gradient
          _this.d3_gradient = _this.d3_svg.append("svg:defs")
                                        .append("svg:radialGradient")
                                        .attr("id", "gradient")
                                        .attr("x1", "0%")
                                        .attr("y1", "0%")
                                        .attr("x2", "100%")
                                        .attr("y2", "100%")
                                        .attr("spreadMethod", "pad");

          // Define the gradient colors
          _this.d3_gradient.append("svg:stop")
                          .attr("offset", "0%")
                          .attr("stop-color", "#00f")
                          .attr("stop-opacity", 1);

          _this.d3_gradient.append("svg:stop")
                          .attr("offset", "100%")
                          .attr("stop-color", "#fff")
                          .attr("stop-opacity", 0);

          // set default data
          _this.d3_data = _this.default_circles;
          // set lat_lng for Leaflet
          _this.d3_data.forEach(function(d) {
              d.LatLng = new L.LatLng(d[0], d[1]);
          });
                          
          // append circle
          var tooltip = d3.select("div#map_tip")
                          .append("div")
                          .style("position", "absolute")
                          .style("z-index", "10000")
                          .style("visibility", "hidden");

          // set circles
          var g = _this.d3_svg.append("g");
          _this.d3_circles = g.selectAll("circle")
                            .data(ctrl.map_handler.d3_data)
                            .enter()
                            .append("circle")
                            .attr('r', 0)
                            .attr('class', 'location_circle')
                            .attr('fill', 'url(#gradient)')
                            .on("mouseover", function(){return tooltip.style("visibility", "visible");})
                            .on("mousemove", function(d){return tooltip.html('tweet location: ' + d).style("top", (d3.event.pageY - 10)+"px").style("left",(d3.event.pageX + 10)+"px");})
                            .on("mouseout", function(){return tooltip.style("visibility", "hidden");});
                          
          // reset view
          _this.map.on( "viewreset", _this.update_d3_elem_on_map.bind(_this) );
          setTimeout( _this.update_d3_elem_on_map.bind(_this), 500 );

          // set map popup
          var map_popup = L.popup();
          map_popup.setLatLng( _this.default_location )
                  .setContent( '<div id="front_page_map">' +
                                '<h3>Facebooc</h3>' +
                                '<p class="lead">The best social network you\'ve never heard of!</p>' +
                                '<p>' +
                                '<a class="btn btn-info signup_btn" href="/signup/" role="button">Sign up today</a>' +
                                '</p>' +
                                '<p class="small">Or <a href="/login/">login</a> if you heard about Facebooc before it was cool.</p>' +
                                '</div>')
                  .openOn(_this.map);
        }
      },
      update_d3_elem_on_map : function(){
        var _this = this;
        _this.d3_circles.attr("transform", 
            function(d) { 
                return "translate("+ 
                _this.map.latLngToLayerPoint(d.LatLng).x +","+ 
                _this.map.latLngToLayerPoint(d.LatLng).y +")";
            }
        ).transition()
        .duration(900)
        .delay(800)
        .attr('r', 100)
        .style("opacity",0)
        .each("end", function(){
            d3.select(this)
              .transition()
              .duration(700)
              .style("r", 20)
              .style("opacity",0.8);
        });
      }
    }
    ctrl.init_front_page = function(){
      console.log('Hello Facebooc Front Page');
      ctrl.map_handler.init_map(null);
    }
  }
  frontPageController.$injector = ['$rootScope', '$scope', '$http', '$window', '$location', 'GLOBAL_VALUES'];
  window.main_app.controller('frontPageCtrl', frontPageController);

})(jQuery); // End of use strict
