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
    }
  }
  headerController.$injector = ['$rootScope', '$scope', '$http', '$window', '$location', 'GLOBAL_VALUES'];
  window.main_app.controller('headerCtrl', headerController);

  // headerController
  var frontPageController = function($rootScope, $scope, $http, $window, $location, GLOBAL_VALUES){
    var ctrl = this;
    ctrl.twitter_map_handler = ctrl.twitter_map_handler || {
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
        if(arg_position && arg_position.coords){
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

          //
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

          var g = _this.d3_svg.append("g");
          _this.d3_circles = g.selectAll("circle")
                            .data(_this.d3_data)
                            .enter()
                            .append("circle")
                            .attr('r', 0)
                            .attr('class', 'location_circle')
                            .attr('fill', 'url(#gradient)')
                            .on("mouseover", function(){return tooltip.style("visibility", "visible");})
                            .on("mousemove", function(d){return tooltip.html('tweet location: ' + d).style("top", (d3.event.pageY - 10)+"px").style("left",(d3.event.pageX + 10)+"px");})
                            .on("mouseout", function(){return tooltip.style("visibility", "hidden");});
                          
          _this.map.on( "viewreset", _this.update_d3_elem_on_map.bind(_this) );
          setTimeout( _this.update_d3_elem_on_map.bind(_this), 500 );

          // set map popup
          var map_popup = L.popup();
          map_popup.setLatLng( _this.default_location )
                  .setContent( '<div style="text-align: center;">' +
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
      if (navigator.geolocation) {
        navigator.geolocation.getCurrentPosition(ctrl.twitter_map_handler.init_map);
      }else { 
        alert("Geolocation is not supported by this browser.");
      }
    }
  }
  frontPageController.$injector = ['$rootScope', '$scope', '$http', '$window', '$location', 'GLOBAL_VALUES'];
  window.main_app.controller('frontPageCtrl', frontPageController);

})(jQuery); // End of use strict
