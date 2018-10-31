var preview_enable = 0;
var clip_enable = 0;
var api_url = '/api/0.1/live_viewer/';
var colormap;

function pollPreview(image_elem)
{
    if (preview_enable) {
       getImage(colormap);
    }
    setTimeout(pollPreview, 100, image_elem);      
}

function configPreview() 
{
    $("[name='preview_enable']").bootstrapSwitch();
    $("[name='preview_enable']").bootstrapSwitch('state', preview_enable, true);
    $('input[name="preview_enable"]').on('switchChange.bootstrapSwitch', function(event,state) {
	    changePreviewEnable();
	});

	$("[name='clip_enable']").bootstrapSwitch();
	$("[name='clip_enable']").bootstrapSwitch('state', clip_enable, true);
    $('input[name="clip_enable"]').on('switchChange.bootstrapSwitch', function(event,state) {
	    changeClipEnable();
	});

}

function changePreviewEnable() 
{
    preview_enable = $("[name='preview_enable']").bootstrapSwitch('state');
}

function changeClipEnable()
{
    clip_enable = $("[name='clip_enable']").bootstrapSwitch('state');
}

function populateDropdown()
{
    let dropdown = $('#colormap_dropdown');

    dropdown.empty();

    var options_count = 0;


    $.getJSON(api_url, function (response)
    {
        colormap = response.colormap_default;
        console.log(response);
        $.each(response.colormap_options, function(key, value){
            console.log("Option: " + value);
            options_count ++;
            var option = new Option(value, key);
            console.log("Comparing " + key + " with " + colormap)
            console.log("Comparison: " + key.localeCompare(colormap))
            var selected = ''
            if(key.localeCompare(colormap) === 0)
            {
                console.log("Selecting option " + options_count +": " + key)
                selected = 'selected="true"'
            }

            dropdown.append('<option ' + selected + ' value=' + key + '>' + value + '</option>');

        })

        console.log(response);
        $('#img_max').val(parseInt(response.data_min_max[1]));
        $('#img_min').val(parseInt(response.data_min_max[0]));
    });
}

function colormap_change(value)
{
    console.log("colormap Changed: " + value)
    $.ajax({
    type: "PUT",
    url: api_url,
    contentType: "application/json",
    data: '{"colormap_selected": "' + value +'" }'
    });
    colormap = value;
    getImage(value);
}

function getImage(colormap)
{

    var options = 'colormap=' + colormap;
    if(clip_enable)
    {
        options += '&clip-min=' + 1000 +
                   '&clip-max=' + 2000;
    }

     //+
//                  '&clip-min=' + 1000 +
//                  '&clip-max=' + 2000;
    var $image = $( '#' + "preview_image" );
    $image.attr("src", $image.attr("data-src")  + '?' + options + '&' +  new Date().getTime());

    $.getJSON(api_url + "data_min_max", function(response)
    {
        $('#img_max').val(parseInt(response.data_min_max[1]));
        $('#img_min').val(parseInt(response.data_min_max[0]));
    });

}

$( document ).ready(function() 
{

    configPreview();
    populateDropdown();
    pollPreview("preview_image");
});
