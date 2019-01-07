var LiveViewApp = (function()
{
    'use strict';

    var liveview_enable = false;
    var clip_enable = false;
    var autosize_enable = true;
    var clip_slider = null;
    var size_slider = null;
    var api_url = '/api/0.1/live_view/';
    var img_elem = null;
    var img_scaling = 1.0;

    var img_start_time = new Date().getTime();
    var img_end_time = null;
    var img_pol_freq = 1;

    var init = function() 
    {

        // Get reference to image element and attach the resize function to its onLoad event
        img_elem = $('#liveview_image');
        img_elem.load(function() 
        {
            img_end_time = new Date().getTime();
            resizeImage();
            var load_time = img_end_time - img_start_time;
            console.log("Image Load Time: " + load_time);

            if(liveview_enable){

                if (img_pol_freq - load_time < 0)
                {
                    console.log("Time elapsed, loading new image");

                    updateImage();
                }
                else
                {
                    console.log("Remaining time for image: " + (img_pol_freq - load_time));
                    setTimeout(updateImage, img_pol_freq - load_time);
                }
                if(load_time < img_pol_freq)
                {
                    $('#frame_rate').text((1000 / img_pol_freq).toFixed(2) + "Hz");
                }
                else
                {
                    $('#frame_rate').text((1000 / load_time).toFixed(2) + "Hz");
                }
            }
        });

        // Configure auto-update switch
        $("[name='liveview_enable']").bootstrapSwitch();
        $("[name='liveview_enable']").bootstrapSwitch('state', liveview_enable, true);
        $('input[name="liveview_enable"]').on('switchChange.bootstrapSwitch', function(event,state) {
            changeLiveViewEnable();
        });

        // Configure clipping enable switch
        $("[name='clip_enable']").bootstrapSwitch();
        $("[name='clip_enable']").bootstrapSwitch('state', clip_enable, true);
        $('input[name="clip_enable"]').on('switchChange.bootstrapSwitch', function(event,state) {
            changeClipEnable();
        });

        // Configure clipping range slider
        clip_slider = $("#clip_range").slider({}).on('slideStop', changeClipEvent);

        // Configure autosizing enable switch
        $("[name='autosize_enable']").bootstrapSwitch();
        $("[name='autosize_enable']").bootstrapSwitch('state', autosize_enable, true);
        $('input[name="autosize_enable"]').on('switchChange.bootstrapSwitch', function(event,state) {
            changeAutosizeEnable();
        });

        // Configure sizing range slider
        size_slider = $("#size_range").slider({}).on('slideStop', changeSizeEvent);
        size_slider.slider(!autosize_enable ? "enable" : "disable");

        // Retrieve API data and populate controls
        $.getJSON(api_url, function (response)
        {
            buildColormapSelect(response.colormap_selected, response.colormap_options);
            updateClipRange(response.data_min_max, true);
            changeClipEnable();
        });

        // Bind the window resize event
        $( window ).on('resize', function()
        {
            if (!liveview_enable) {
                resizeImage();
            }
        });

        updateImage();
    };

    var buildColormapSelect = function(selected_colormap, colormap_options)
    {
        let dropdown = $('#colormap_select');

        dropdown.empty();

        var colormaps_sorted = Object.keys(colormap_options).sort();

        $.each(colormaps_sorted, function(idx, colormap) {
            var colormap_name = colormap_options[colormap];
            var selected = '';
            if (colormap.localeCompare(selected_colormap) === 0)
            {
                selected = 'selected="true"'
            }
            dropdown.append('<option ' + selected + 
                ' value=' + colormap + '>' + colormap_name + '</option>');
        });

        dropdown.change(function()
        {
            changeColormap(this.value);
        });
    };

    var changeLiveViewEnable = function() 
    {
        liveview_enable = $("[name='liveview_enable']").bootstrapSwitch('state');
        if(liveview_enable)
        {
            updateImage();
        }
    };

    var updateClipRange = function(data_min_max, reset_current=false)
    {
        var data_min = parseInt(data_min_max[0]);
        var data_max = parseInt(data_min_max[1]);

        $('#clip_min').text(data_min);
        $('#clip_max').text(data_max);

        var current_values = clip_slider.data('slider').getValue();

        if (reset_current) {
            current_values = [data_min, data_max]
        }

        clip_slider.slider('setAttribute', 'max', data_max);
        clip_slider.slider('setAttribute', 'min', data_min);
        clip_slider.slider('setValue', current_values);

    };

    var changeClipEnable = function()
    {
        clip_enable = $("[name='clip_enable']").bootstrapSwitch('state');
        clip_slider.slider(clip_enable ? "enable" : "disable");
        changeClipRange();
    };

    var changeClipEvent = function(slide_event)
    {
        var new_range = slide_event.value;
        changeClipRange(new_range);
    };

    var changeClipRange = function(clip_range=null) {
        
        if (clip_range === null)
        {
            clip_range = [
                clip_slider.slider('getAttribute', 'min'),
                clip_slider.slider('getAttribute', 'max')
            ]
        }
        if (!clip_enable)
        {
            clip_range = [null, null]
        }

        console.log('Clip range changed: min=' + clip_range[0] + " max=" + clip_range[1]);
        $.ajax({
            type: "PUT",
            url: api_url,
            contentType: "application/json",
            data: JSON.stringify({"clip_range": clip_range})
        });

    };

    var changeAutosizeEnable = function()
    {
        autosize_enable = $("[name='autosize_enable']").bootstrapSwitch('state');
        size_slider.slider(!autosize_enable ? "enable" : "disable");
        resizeImage();
    };

    var changeSizeEvent = function(size_event)
    {
        var new_size = size_event.value;
        img_scaling = new_size / 100;
        resizeImage();
    };

    var changeColormap = function(value)
    {
        console.log("Colormap changed to " + value)
        $.ajax({
            type: "PUT",
            url: api_url,
            contentType: "application/json",
            data: '{"colormap_selected": "' + value +'" }'
        });
        updateImage();
    }

    var updateImage = function()
    {
        img_start_time = new Date().getTime();
        img_elem.attr("src", img_elem.attr("data-src") + '?' +  new Date().getTime());

        $.getJSON(api_url + "data_min_max", function(response)
        {
            updateClipRange(response.data_min_max, !clip_enable);
        });
    };

    var resizeImage = function()
    {

        var img_width = img_elem.naturalWidth();
        var img_height = img_elem.naturalHeight();

        if (autosize_enable) {

            var img_container_width =  $("#liveview_container").width();
            var img_container_height = $("#liveview_container").height();

            var width_scaling = Math.min(img_container_width / img_width, 1.0);
            var height_scaling = Math.min(img_container_height / img_height, 1.0);

            img_scaling = Math.min(width_scaling, height_scaling);
            size_slider.data('slider').setValue(Math.floor(img_scaling * 100));
        }

        img_elem.width( Math.floor(img_scaling * img_width));
        img_elem.height(Math.floor(img_scaling * img_height));

    };

    return {
        init: init,
    };

})();

$( document ).ready(function() 
{
    LiveViewApp.init();
});

// Generate naturalWidth/naturalHeight methods for images in JQuery
(function($){
    var
    props = ['Width', 'Height'],
    prop;

    while (prop = props.pop()) {
        (function (natural, prop) {
            $.fn[natural] = (natural in new Image()) ?
            function () {
                return this[0][natural];
            } :
            function () {
                var
                node = this[0],
                img,
                value;

                if (node.tagName.toLowerCase() === 'img') {
                    img = new Image();
                    img.src = node.src,
                    value = img[prop];
                }
                return value;
            };
        }('natural' + prop, prop.toLowerCase()));
    }
}(jQuery));
