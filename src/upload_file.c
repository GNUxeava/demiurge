/*
  This file is part of demiurge.

  demiurge is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  demiurge is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with demiurge.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <json-c/json.h>
#include "asprintf.h"
#include "util.h"

int
upload_file(const char *path, const char *description, char **id_ptr)
{
	char instance[50];
	char client_id[50];
	char client_secret[50];
	char access_token[50];

	struct json_object *parsed_json;
	struct json_object *json_media_id;
	char buf[8192];

	get_tokens_from_file(
		".demiurgerc", instance, client_id, client_secret, access_token);
	CURL *curl = curl_easy_init();
	if(curl == NULL) {
		fprintf(stderr, "Error creating libcurl thing\n");
		return -1;
	}
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, buf);

	char *url_to_post = NULL;
	asprintf(&url_to_post, "%s/api/v1/media", instance);

	/* Don't repeat yourself, so they say, it's the root of all evil
	 * today */

	char *header_fmt = "Authorization: Bearer %s";
	struct curl_slist *header_list = NULL;
	char *authorization_header = NULL;
	asprintf(&authorization_header, header_fmt, access_token);
	header_list = curl_slist_append(header_list, authorization_header);

	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_list);

	curl_mime *mime;
	mime = curl_mime_init(curl);
	curl_mimepart *image_part;
	image_part = curl_mime_addpart(mime);
	curl_mimepart *description_part;
	description_part = curl_mime_addpart(mime);
	/* Upload file */

	curl_mime_filedata(image_part, path);
	curl_mime_name(image_part, "file");

	/* Description */

	curl_mime_data(description_part, description, CURL_ZERO_TERMINATED);
	curl_mime_name(description_part, "description");

	/* Post */

	curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
	curl_easy_setopt(curl, CURLOPT_URL, url_to_post);

	/* Free the things */
	curl_easy_perform(curl);
	free(url_to_post);
	curl_easy_cleanup(curl);
	curl_mime_free(mime);
	curl_slist_free_all(header_list);
	/* Get the media id */
	parsed_json = json_tokener_parse(buf);
	json_object_object_get_ex(parsed_json, "id", &json_media_id);
	const char *media_id = json_object_get_string(json_media_id);
	*id_ptr = (char *)media_id;
	free(parsed_json);

	return 0;
}
