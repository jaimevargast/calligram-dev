// Comfortable background colors:
var background_colors = ["#84A5DD","#64d6e2","#FDAED4","#785ebb","#a09de5",
                         "#A0CADB","#79BBB5","#F6D860","#9988CD","#EDB948",
                         "#FDACB4","#dbbe39","#DFBC94","#3CCAD1","#F68F6F",
                         "#9787EA","#ccc5e3","#FD9372","#9B7AD5","#de89ac",
                         "#98BFF6","#FEC54F","#9784ED","#F4C3C5","#63C5AB",
                         "#F4696B","#E794AE","#9B7FE6","#EF9F64","#87C4A3"];

function randomBackgroundColor(){
    return background_colors[Math.floor(Math.random()*background_colors.length)];
}

// Kangaroo example path:
var exampleShape ="m209.041,221.048c-0.282,6.054 -1.237,12.092 -0.71899,18.15199c0.437,5.103\
                 2.99699,7.761 7.97899,8.992c9.56601,-1.49699 18.936,-0.55299 28.11101,2.83c3.894,1.92999 3.27299,6.78\
                 1.23199,9.849c-2.83499,4.263 -6.59399,7.771 -11.41,9.672c-9.903,3.90799 -22.96799,-1.431 -32.058,\
                 -5.55499c-19.108,-8.668 -16.836,-42.729 -16.245,-59.668c-0.39,-2.92601 -1.424,-3.06999 -3.099,\
                 -0.43401c-3.16299,16.283 -11.30701,35.39401 -7.69,52.244c1.713,7.98199 13.922,12.785 18.274,\
                 19.46402c3.627,5.56598 9.854,1.65598 9.952,8.32397c0.073,4.983 -2.49899,7.36801 -6.86699,\
                 8.914c-9.90401,3.508 -19.00801,1.41 -26.106,-6.33701c-15.05901,-16.436 -15.26601,-89.83798 -23.72,\
                 -93.61699c-35.094,-15.69 -41.041,-54.98799 -71.076,-73.37299c-24.879,-15.23 -45.292,33.945 -68.494,\
                 4.277c-7.62,-9.744 26.34,-59.254 35.385,-65.416c22.371,-15.24 -4.95,-17.751 2.097,-42.56601c4.791,\
                 -16.86899 23.862,18.06101 26.127,18.355c1.62701,0.211 38.39101,-52.688 27.819,-5.689c-6.027,\
                 26.79001 -11.393,34.799 15.01701,52.51801c21.741,14.588 47.25,8.961 70.41899,1.191c38.86501,\
                 -13.035 77.37799,-21.516 118.48502,-14.596c72.83698,12.261 134.25299,61.73599 205.75,80.394c70.39597,\
                 18.37099 170.59396,14.655 223.92502,-43.422c1.56995,-1.71101 19.23395,-33.437 19.65698,-22.525c0.69,\
                 17.764 -6.20001,33.004 -15.66101,47.412c-17.992,27.396 -43.20398,45.33 -73.82001,56.03099c-48.565,\
                 16.974 -101.52698,25.767 -152.65298,17.64c-16.80002,-2.67 -83.12701,-28.80499 -98.129,-14.679c-2.59201,\
                 2.44101 2.13699,22.19701 2.18701,25.095c0.33798,19.76001 -24.73602,21.14401 -38.41,18.48601c-5.79602,\
                 -1.127 -8.45502,0.321 -10.66501,5.88699c-16.64401,41.93102 17.16599,73.29301 26.88599,112.088c6.93399,\
                 27.67999 -49.76199,32.64102 -66.48401,38.07602c-14.71198,4.78198 -76.84198,42.966 -87.95699,\
                 30.66299c-1.463,-1.61801 -0.37,-3.534 0.17999,-5.29501c8.619,-27.573 36.10201,-36.05399 59.48201,\
                 -46.23099c4.435,-1.92999 49.88998,-9.181 51.34399,-13.19699c5.004,-13.83502 -32.56799,-67.68903 -40.54099,\
                 -80.09601c-12.828,-19.96301 -35.71802,-54.009 -58.31102,-63.22801c-11.765,-4.79999 -24.52699,-5.45099 -36.297,\
                 -10.10199c-2.911,-1.15001 -3.82899,-0.502 -3.87399,2.67999c-0.07701,5.59601 -0.022,11.194 -0.022,16.79201c-0.28,6.054 0,-5.59801 0,0z";

var menuState = {state: "hidden"};

var selectedShape = {name: "", geometry: ""};

var userStroke = {};

var userCanvas = {};

// Check and replace missing control object:
if (typeof Control == 'undefined')
{
    var ControlObject = function(){};
    ControlObject.prototype.callableEverywhere = function (){return -1;};
    ControlObject.prototype.receiveData = function(data){};
    var Control = new ControlObject;
}
