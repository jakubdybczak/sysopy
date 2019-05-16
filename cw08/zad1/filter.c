#include "image.h"
#include "filter.h"
#include "errors.h"
#include "utils.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define eps 0.000001

int filter_normalize(filter_t *filter)
{
    int sum = 0;
    int num = filter->size * filter->size;
    for (int i = 0; i < num; i++)
        sum += filter->array[i];

    if (sum == 0)
    {
        filter->normalized = 1;
        return 0;
    }

    for (int i = 0; i < num; i++)
        filter->array[i] /= sum;

    filter->normalized = 1;
    return 0;
}

int filter_load(const char *path, filter_t *filter)
{
    FILE *fd;
    if ((fd = fopen(path, "r")) == NULL)
    {
        return err("Cannot open image", -1);
    }
    char buffer[1024];
    int main_line = 0;
    int index = 0;
    while (fgets(buffer, 1024, fd) != NULL)
    {
        if (buffer[0] == '#')
            continue;

        if (main_line == 0)
        {
            int size = 0;
            if (parse_num(buffer, &size) < 0)
                return -1;
            filter->size = size;
            filter->array = malloc(filter->size * filter->size * sizeof(float));
        }
        else
        {
            char *ptr = strtok(buffer, " \r\n");
            while (ptr != NULL)
            {
                int tmp;
                if (parse_num(ptr, &tmp) < 0)
                {
                    return err("Cannot open file (unknown image format)", -1);
                }
                filter->array[index++] = tmp;
                ptr = strtok(NULL, " \r\n");
            }
        }
        main_line++;
    }

    if (index != (filter->size * filter->size))
        return err("Cannot open file (unknown image format)", -1);

    int sum = 0;
    int num = filter->size * filter->size;
    for (int i = 0; i < num; i++)
        sum += filter->array[i];

    if (abs(sum - 0.0) < eps || abs(sum - 1.0) < eps)
        filter->normalized = 1;
    else
        filter->normalized = 0;

    fclose(fd);
    return 0;
}

static int transform_coords(int *x, int *y, img_t *image, edge_mode_t edge_mode)
{
    switch (edge_mode)
    {
    case EDGE_WRAP:
        if (*x < 0)
            *x = 0;
        if (*y < 0)
            *y = 0;
        if (*x >= image->width)
            *x = image->width - 1;
        if (*y >= image->height)
            *x = image->height - 1;
        break;
    default:
        return err("Unknown edge mode", -1);
        break;
    }
    return 0;
}

int filter_apply(filter_t *filter, img_t *image_in, img_t *image_out, int x, int y, edge_mode_t edge_mode)
{
    if (filter->normalized == 0)
        return err("Filter not normalized", -1);

    float pixel_values[3];
    for (int i = 0; i < 3; i++)
        pixel_values[i] = 0;

    int left_offset = -filter->size / 2;
    int right_offset = filter->size / 2;
    if (filter->size % 2 == 0)
        right_offset--;

    int filter_index = 0;
    for (int yp = y + left_offset; yp <= y + right_offset; yp++)
        for (int xp = x + left_offset; xp <= x + right_offset; xp++)
        {
            int xp2 = xp;
            int yp2 = yp;
            if (transform_coords(&xp2, &yp2, image_in, edge_mode) < 0)
                return -1;

            int pixel_index;
            img_getpixelindex(image_in, xp2, yp2, &pixel_index);

            switch (image_in->color_mode)
            {
            case COLOR_BW:
            case COLOR_GRAY:
                pixel_values[0] += filter->array[filter_index] * image_in->array[pixel_index];
                break;
            case COLOR_RGB:
                for (int i = 0; i < 3; i++)
                    pixel_values[i] += filter->array[filter_index] * image_in->array[pixel_index + i];
                break;
            default:
                return err("Error", -1);
                break;
            }

            filter_index++;
        }
    for (int i = 0; i < 3; i++)
    {
        if (pixel_values[i] < 0)
            pixel_values[i] = 0;
        if (pixel_values[i] > image_out->max_value)
            pixel_values[i] = image_out->max_value;
    }

    int out_pixel_index;
    img_getpixelindex(image_out, x, y, &out_pixel_index);

    switch (image_in->color_mode)
    {
    case COLOR_BW:
    case COLOR_GRAY:
        image_out->array[out_pixel_index] = (int)pixel_values[0];
        break;
    case COLOR_RGB:
        for (int i = 0; i < 3; i++)
            image_out->array[out_pixel_index + i] = (int)pixel_values[i];
        break;
    default:
        return err("Error", -1);
        break;
    }

    // printf("%f %d\n", pixel_value, image_out->array[out_pixel_index]);
    return 0;
}