var liveview_enable = 0;
var clip_enable = 0;
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
    // Configure switch elements
    $("[name='liveview_enable']").bootstrapSwitch();
    $("[name='liveview_enable']").bootstrapSwitch('state', liveview_enable, true);
    $('input[name="liveview_enable"]').on('switchChange.bootstrapSwitch', function(event,state) {
	    changeLiveViewEnable();
	});

	$("[name='clip_enable']").bootstrapSwitch();
	$("[name='clip_enable']").bootstrapSwitch('state', clip_enable, true);
    $('input[name="clip_enable"]').on('switchChange.bootstrapSwitch', function(event,state) {
	    changeClipEnable();
	});

    // Retrieve API data and populate controls
    $.getJSON(api_url, function (response)
    {
        buildColormapDropdown(response.colormap_selected, response.colormap_options);
        updateDataMinMax(response.data_min_max);
    });

}

function buildColormapDropdown(selected_colormap, colormap_options)
{
    let dropdown = $('#colormap_dropdown');

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
 
function updateDataMinMax(data_min_max)
{
    $('#img_min').text(parseInt(data_min_max[0]));
    $('#img_max').text(parseInt(data_min_max[1]));
}

function changeLiveViewEnable() 
{
    liveview_enable = $("[name='liveview_enable']").bootstrapSwitch('state');
}

function changeClipEnable()
{
    clip_enable = $("[name='clip_enable']").bootstrapSwitch('state');
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
        updateDataMinMax(response.data_min_max);
    });

}

$( document ).ready(function() 
{
    configureLiveView();
    pollLiveView(image_element);
});
