#include <lunaix/block.h>
#include <lunaix/fs/twifs.h>

static struct twifs_node* blk_root;

void
blk_mapping_init()
{
    blk_root = twifs_dir_node(NULL, "block");
}

void
__blk_rd_size(struct twimap* map)
{
    struct block_dev* bdev = twimap_data(map, struct block_dev*);
    size_t secsize = bdev->blk_size;
    twimap_printf(map, "%u", bdev->end_lba * secsize);
}

void
__blk_rd_lblksz(struct twimap* map)
{
    struct block_dev* bdev = twimap_data(map, struct block_dev*);
    size_t lblksz = bdev->blk_size;
    twimap_printf(map, "%u", lblksz);
}

void
__blk_rd_name(struct twimap* map)
{
    struct block_dev* bdev = twimap_data(map, struct block_dev*);
    twimap_printf(map, "%s", bdev->name);
}

void
blk_set_blkmapping(struct block_dev* bdev, void* fsnode)
{
    struct twifs_node* dev_root = (struct twifs_node*)fsnode;

    struct twimap* map = twifs_mapping(dev_root, bdev, "size");
    map->read = __blk_rd_size;

    map = twifs_mapping(dev_root, bdev, "lblk_size");
    map->read = __blk_rd_lblksz;

    map = twifs_mapping(dev_root, bdev, "name");
    map->read = __blk_rd_name;
}