select add_media((select max(event_id) from event), (select max(media_description_id) from media_description), 'identifier1', 1, 25, 125);
