#ifndef _ASM_X86_IOMMU_H
#define _ASM_X86_IOMMU_H

static inline void iommu_shutdown_noop(void) {}
extern void no_iommu_init(void);
extern struct dma_map_ops nommu_dma_ops;
extern int force_iommu, no_iommu;
extern int iommu_detected;
extern int iommu_pass_through;

/* 10 seconds */
#define DMAR_OPERATION_TIMEOUT ((cycles_t) tsc_khz*10*1000)

#endif /* _ASM_X86_IOMMU_H */
