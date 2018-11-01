var liveview_enable = 0;
var clip_enable = 0;
var clip_slider = null;
var api_url = '/api/0.1/live_view/';
var image_element = "liveview_image";

function pollLiveView(image_elem)
{
    if (liveview_enable) {
       getImage(image_elem);
    }
    setTimeout(pollLiveView, 100, image_elem);      
}

function configureLiveView() 
{
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

    // Retrieve API data and populate controls
    $.getJSON(api_url, function (response)
    {
        buildColormapSelect(response.colormap_selected, response.colormap_options);
        updateClipRange(response.data_min_max, true);
        changeClipEnable();
    });

}

function buildColormapSelect(selected_colormap, colormap_options)
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
}
 
function updateClipRange(data_min_max, reset_current=false)
{
    var data_min = parseInt(data_min_max[0]);
    var data_max = parseInt(data_min_max[1]);

    $('#clip_min').text(data_min);
    $('#clip_max').text(data_max);

    //var clip_slider = $("#clip_range");
    var current_values = clip_slider.data('slider').getValue();

    if (reset_current) {
        current_values = [data_min, data_max]
    }

    clip_slider.slider('setAttribute', 'max', data_max);
    clip_slider.slider('setAttribute', 'min', data_min);
    clip_slider.slider('setValue', current_values);

}

function changeLiveViewEnable() 
{
    liveview_enable = $("[name='liveview_enable']").bootstrapSwitch('state');
}

function changeClipEnable()
{
    clip_enable = $("[name='clip_enable']").bootstrapSwitch('state');
    clip_slider.slider(clip_enable ? "enable" : "disable");
    changeClipRange();
}

function changeClipEvent(slide_event)
{
    var new_range = slide_event.value;
    changeClipRange(new_range);
}

function changeClipRange(clip_range=null) {
    
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

}

function changeColormap(value)
{
    console.log("Colormap changed to " + value)
    $.ajax({
        type: "PUT",
        url: api_url,
        contentType: "application/json",
        data: '{"colormap_selected": "' + value +'" }'
    });
    getImage(image_element);
}

function getImage(image_elem)
{

    var $image = $('#' + image_elem);
    $image.attr("src", $image.attr("data-src") + '?' +  new Date().getTime());

    $.getJSON(api_url + "data_min_max", function(response)
    {
        updateClipRange(response.data_min_max, !clip_enable);
    });

}

$( document ).ready(function() 
{
    configureLiveView();
    pollLiveView(image_element);
});
