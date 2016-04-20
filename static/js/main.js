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
      map : undefined,
      tiles : undefined,
      my_icon : undefined,
      d3_svg : undefined,
      d3_gradient : undefined,
      d3_data : undefined,
      d3_circles : undefined,
      is_set_map_center_on : true,
      default_location : [23.893589, 121.083589],
      defualt_circles : [[23.893589, 121.083589]],
      location_ary : [],
      tweets_list : [],
      is_appending : false,
      init_map : function(){
        // init map
        if(ctrl.twitter_map_handler.map === undefined){
          // init map
          ctrl.twitter_map_handler.map = new L.map('front_page')
                          .setView( ctrl.twitter_map_handler.default_location, 8);

          // set map tiles
          L.tileLayer(
            'http://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
            attribution: '© + Openstreetmap Contributors',
            maxZoom: 18,
          }).addTo(ctrl.twitter_map_handler.map);

          //
          ctrl.twitter_map_handler.map._initPathRoot();

          // set d3
          if(ctrl.twitter_map_handler.d3_svg === undefined){
            ctrl.twitter_map_handler.d3_svg = d3.select("div#front_page").select("svg");
          }

          // define the gradient
          ctrl.twitter_map_handler.d3_gradient = ctrl.twitter_map_handler.d3_svg.append("svg:defs")
                                        .append("svg:radialGradient")
                                        .attr("id", "gradient")
                                        .attr("x1", "0%")
                                        .attr("y1", "0%")
                                        .attr("x2", "100%")
                                        .attr("y2", "100%")
                                        .attr("spreadMethod", "pad");

          // Define the gradient colors
          ctrl.twitter_map_handler.d3_gradient.append("svg:stop")
                          .attr("offset", "0%")
                          .attr("stop-color", "#00f")
                          .attr("stop-opacity", 1);

          ctrl.twitter_map_handler.d3_gradient.append("svg:stop")
                          .attr("offset", "100%")
                          .attr("stop-color", "#fff")
                          .attr("stop-opacity", 0);

          // set default data
          ctrl.twitter_map_handler.d3_data = ctrl.twitter_map_handler.defualt_circles;
          // set lat_lng for Leaflet
          ctrl.twitter_map_handler.d3_data.forEach(function(d) {
              d.LatLng = new L.LatLng(d[0], d[1]);
          });
                          
          // append circle
          var tooltip = d3.select("div#map_tip")
                          .append("div")
                          .style("position", "absolute")
                          .style("z-index", "10000")
                          .style("visibility", "hidden");

          var g = ctrl.twitter_map_handler.d3_svg.append("g");
          ctrl.twitter_map_handler.d3_circles = g.selectAll("circle")
                            .data(ctrl.twitter_map_handler.d3_data)
                            .enter()
                            .append("circle")
                            .attr('r', 0)
                            .attr('class', 'location_circle')
                            .attr('fill', 'url(#gradient)')
                            .on("mouseover", function(){return tooltip.style("visibility", "visible");})
                            .on("mousemove", function(d){return tooltip.html('tweet location: ' + d).style("top", (d3.event.pageY - 10)+"px").style("left",(d3.event.pageX + 10)+"px");})
                            .on("mouseout", function(){return tooltip.style("visibility", "hidden");});
                          
          ctrl.twitter_map_handler.map.on( "viewreset", ctrl.twitter_map_handler.update_d3_elem_on_map.bind(ctrl.twitter_map_handler) );

          //
          setTimeout( ctrl.twitter_map_handler.update_d3_elem_on_map.bind(ctrl.twitter_map_handler), 500 );

          // set map popup
          var map_popup = L.popup();
          map_popup.setLatLng( [ 23.893589, 121.083589] )
                  .setContent( '<div style="text-align: center;">' +
                               '<p>Track 2016 TW Election on Twitter</p>' +
                               '<img style="width: 80px;" src="/static/leaflet-0.7/images/twitter-1.png"><br>' +
                               '</div>')
                  .openOn(ctrl.twitter_map_handler.map);
        }
      }
    }
    ctrl.init_front_page = function(){
      console.log('Hello Facebooc');
      ctrl.twitter_map_handler.init_map();
    }
  }
  frontPageController.$injector = ['$rootScope', '$scope', '$http', '$window', '$location', 'GLOBAL_VALUES'];
  window.main_app.controller('frontPageCtrl', frontPageController);

})(jQuery); // End of use strict