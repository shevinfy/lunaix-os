#include <hal/ahci/hba.h>
#include <hal/ahci/utils.h>
#include <klibc/string.h>

#define IDDEV_OFFMAXLBA 60
#define IDDEV_OFFMAXLBA_EXT 230
#define IDDEV_OFFLSECSIZE 117
#define IDDEV_OFFWWN 108
#define IDDEV_OFFSERIALNUM 10
#define IDDEV_OFFMODELNUM 27
#define IDDEV_OFFADDSUPPORT 69

void
ahci_parse_dev_info(struct hba_device* dev_info, uint16_t* data)
{
    dev_info->max_lba = *((uint32_t*)(data + IDDEV_OFFMAXLBA));
    dev_info->block_size = *((uint32_t*)(data + IDDEV_OFFLSECSIZE));
    dev_info->cbd_size = (*data & 0x3) ? 16 : 12;
    dev_info->wwn = *(uint64_t*)(data + IDDEV_OFFWWN);

    if (!dev_info->block_size) {
        dev_info->block_size = 512;
    }

    if ((*(data + IDDEV_OFFADDSUPPORT) & 0x8)) {
        dev_info->max_lba = *((uint64_t*)(data + IDDEV_OFFMAXLBA_EXT));
        dev_info->flags |= HBA_DEV_FEXTLBA;
    }

    ahci_parsestr(&dev_info->serial_num, data + IDDEV_OFFSERIALNUM, 10);
    ahci_parsestr(&dev_info->model, data + IDDEV_OFFMODELNUM, 20);
}

void
ahci_parsestr(char* str, uint16_t* reg_start, int size_word)
{
    int j = 0;
    for (int i = 0; i < size_word; i++, j += 2) {
        uint16_t reg = *(reg_start + i);
        str[j] = (char)(reg >> 8);
        str[j + 1] = (char)(reg & 0xff);
    }
    str[j - 1] = '\0';
}